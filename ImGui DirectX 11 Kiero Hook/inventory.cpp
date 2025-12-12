#include "inventory.h"
#include "interfaces.h"
#include "logger.h"
#include <algorithm>

static std::vector<DumpedItem_t> vecDumpedItems;
static DumpedItem_t* pSelectedItem = nullptr;

static bool is_paint_kit_for_weapon(C_PaintKit* paint_kit, C_EconItemDefinition* weapon)
{
    // VALIDAÇÃO: Verificar ponteiros antes de usar
    if (!paint_kit) {
        LOG_ERROR("is_paint_kit_for_weapon: paint_kit is NULL");
        return false;
    }
    
    if (!weapon) {
        LOG_ERROR("is_paint_kit_for_weapon: weapon is NULL");
        return false;
    }

    const char* name = weapon->get_simple();
    if (!name || name[0] == '\\0') {
        LOG_WARNING("is_paint_kit_for_weapon: weapon->get_simple() returned NULL or empty");
        return false;
    }

    if (!paint_kit->m_name || paint_kit->m_name[0] == '\\0') {
        LOG_WARNING("is_paint_kit_for_weapon: paint_kit->m_name is NULL or empty");
        return false;
    }

    std::string path = "panorama/images/econ/default_generated/";
    path += name;
    path += "_";
    path += paint_kit->m_name;
    path += "_light_png.vtex_c";
    
    // VALIDAÇÃO: Verificar file_system antes de usar
    if (!I::file_system) {
        LOG_ERROR("is_paint_kit_for_weapon: I::file_system is NULL");
        return false;
    }
    
    if (I::file_system->exists(path.c_str(), "GAME")) {
        return true;
    }

    return false;
}


bool skinsDump() {
    LOG_INFO("=== Starting skinsDump ===");
    
    vecDumpedItems.clear();
    
    // VALIDAÇÃO: Verificar interfaces críticas
    if (!I::iclient) {
        LOG_ERROR("skinsDump: I::iclient is NULL");
        return false;
    }
    
    auto econ_system = I::iclient->get_econ_item_system();
    if (!econ_system) {
        LOG_ERROR("skinsDump: get_econ_item_system() returned NULL");
        return false;
    }
    
    auto item_schema = econ_system->get_econ_item_schema();
    if (!item_schema)
    {
        LOG_ERROR("skinsDump: get_econ_item_schema() returned NULL");
        return false;
    }
    
    LOG_INFO("skinsDump: Item schema obtained successfully");
    
    int i = 0;
    if (vecDumpedItems.empty())
    {
        const auto& vecItems = item_schema->GetSortedItemDefinitionMap();
        LOG_INFO("skinsDump: Processing items from item definition map");

        const auto& vecPaintKits = item_schema->GetPaintKits();
        LOG_INFO("skinsDump: Processing paint kits");

        const auto& vecStickers = item_schema->GetStickers();
        LOG_DEBUG("skinsDump: Processing stickers");

        std::vector<void*> vecItemss;
        for (auto it = vecItems.begin(); it != vecItems.end(); ++it) {
            vecItemss.push_back(it->m_value); 
        }

        int processedItems = 0;
        int skippedItems = 0;
        
        for (const auto& it : vecItems)
        {
            C_EconItemDefinition* pItem = it.m_value;
            if (!pItem) {
                skippedItems++;
                continue;
            }
            
            const bool isWeapon = pItem->is_weapon();
            const bool isKnife = pItem->is_knife(true);
            const bool isGloves = pItem->is_glove(true);
            const bool IsAgent = pItem->is_agent();
            const bool isOther = !isKnife && !isWeapon && !IsAgent && !isGloves;

            if (!isOther && !isKnife && !isWeapon && !IsAgent && !isGloves) {
                skippedItems++;
                continue;
            }
            
            const char* itemBaseName = pItem->m_pszItemBaseName;

            if (!itemBaseName || itemBaseName[0] == '\\0') {
                skippedItems++;
                continue;
            }
            
            const uint16_t defIdx = pItem->m_nDefIndex;

            DumpedItem_t dumpedItem;
            
            // VALIDAÇÃO: Verificar localize antes de usar
            if (!I::localize) {
                LOG_ERROR("skinsDump: I::localize is NULL");
                return false;
            }
            
            dumpedItem.m_name = I::localize->find_key(pItem->m_pszItemBaseName);

            if (dumpedItem.m_name[0] == '#' || dumpedItem.m_name == "") {
                skippedItems++;
                continue;
            }
            
            dumpedItem.m_defIdx = defIdx;
            dumpedItem.m_rarity = pItem->GetRarity();
            
            if (isWeapon || isKnife || isGloves || IsAgent || isOther)
            {
                const char* simple = pItem->get_simple();
                if (simple && simple[0] != '\\0')
                    dumpedItem.m_simpleName = simple;
            }
            
            dumpedItem.isWeapon = isWeapon;
            dumpedItem.isKnife = isKnife;
            dumpedItem.isGloves = isGloves;
            dumpedItem.isAgent = IsAgent;
            dumpedItem.isOther = isOther;

            if (isKnife || isGloves || isWeapon || IsAgent || isOther)
            {
                dumpedItem.m_unusualItem = true;
            }

            if (IsAgent) {
                dumpedItem.m_dumpedSkins.emplace_back("Vanilla", 0, pItem->GetRarity());
                LOG_DEBUG("skinsDump: Added agent - %s (DefIdx: %d)", dumpedItem.m_name.c_str(), defIdx);
            }
            else if (isOther) {
                dumpedItem.m_dumpedSkins.emplace_back("Vanilla", 0, pItem->GetRarity());
            }
            i++;

            // Processar paint kits para armas/facas/luvas
            if (!IsAgent && !isOther)
            {
                int paintkitsProcessed = 0;
                for (const auto& it : vecPaintKits)
                {
                    C_PaintKit* pPaintKit = it.m_value;
                    if (!pPaintKit || pPaintKit->m_id == 0 || pPaintKit->m_id == 9001)
                        continue;

                    if (!is_paint_kit_for_weapon(pPaintKit, pItem))
                        continue;

                    DumpedSkin_t dumpedSkin;
                    dumpedSkin.m_name = I::localize->find_key(pPaintKit->m_description_tag);
                    if (dumpedSkin.m_name[0] == '#' || dumpedSkin.m_name == "")
                        continue;
                    dumpedSkin.m_ID = pPaintKit->m_id;
                    dumpedSkin.legacy = pPaintKit->legacy();
                    
                    if (pPaintKit->m_name)
                        dumpedSkin.m_tokenName = pPaintKit->m_name;

                    dumpedSkin.m_rarity = std::clamp<int>(
                        static_cast<int>(pItem->GetRarity() + pPaintKit->m_rarity - 1),
                        0,
                        (pPaintKit->m_rarity == 7) ? 7 : 6
                    );
                    dumpedItem.m_dumpedSkins.emplace_back(dumpedSkin);
                    paintkitsProcessed++;
                }
                
                if (paintkitsProcessed > 0) {
                    LOG_DEBUG("skinsDump: Added %d paint kits for %s", paintkitsProcessed, dumpedItem.m_name.c_str());
                }
            }

            // Ordenar skins por raridade
            if (!dumpedItem.m_dumpedSkins.empty() && isWeapon) {
                std::sort(dumpedItem.m_dumpedSkins.begin(),
                    dumpedItem.m_dumpedSkins.end(),
                    [](const DumpedSkin_t& a, const DumpedSkin_t& b) {
                        return a.m_rarity > b.m_rarity;
                    });
            }

            vecDumpedItems.emplace_back(dumpedItem);
            processedItems++;
        }

        LOG_INFO("skinsDump: Processed %d items, skipped %d items", processedItems, skippedItems);

        if (vecDumpedItems.empty()) {
            LOG_ERROR("skinsDump: No items found to dump");
            return false;
        }
        
        LOG_INFO("skinsDump: Successfully dumped %zu items", vecDumpedItems.size());
        return true;
    }
    
    LOG_WARNING("skinsDump: vecDumpedItems is not empty, skipping dump");
    return false;
}
namespace INVENTORY {
    static bool dumped = false;
    
    bool Dump() {
        LOG_INFO("INVENTORY::Dump() called");
        
        if (dumped) {
            LOG_INFO("INVENTORY::Dump() - Already dumped, returning false");
            return false;
        }
        
        if (skinsDump()) {
            dumped = true;
            LOG_INFO("INVENTORY::Dump() - Successfully dumped inventory");
            return true;
        }
        
        LOG_ERROR("INVENTORY::Dump() - Failed to dump inventory");
        return false;
    }

    std::vector<DumpedItem_t>& GetDumpedItems()
    {
        LOG_DEBUG("INVENTORY::GetDumpedItems() called - returning %zu items", vecDumpedItems.size());
        return vecDumpedItems;
    }
    
    bool IsDumped()
    {
        LOG_DEBUG("INVENTORY::IsDumped() - dumped = %s", dumped ? "true" : "false");
        return dumped;
    }
}