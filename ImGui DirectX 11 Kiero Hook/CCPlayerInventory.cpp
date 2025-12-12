#include "CCPlayerInventory.h"
#include "logger.h"
#include <utility>
#include "i_client.h"
#include "memory.h"
#include "utlvector.h"
CCSInventoryManager* CCSInventoryManager::GetInstance() {
    LOG_DEBUG("CCSInventoryManager::GetInstance() called");
    
    using FnGetInstanceS = CCSInventoryManager * (__fastcall*)();
    static FnGetInstanceS oGetInstanceS = reinterpret_cast<FnGetInstanceS>(M::FindPattern(L"client.dll", "48 8d 05 ?? ?? ?? ?? c3 cc cc cc cc cc cc cc cc 8b 91 ?? ?? ?? ?? b8"));

    // VALIDAÇÃO: Verificar se o pattern foi encontrado
    if (!oGetInstanceS) {
        LOG_ERROR("CCSInventoryManager::GetInstance() - Pattern not found");
        return nullptr;
    }

    auto instance = oGetInstanceS();
    if (!instance) {
        LOG_ERROR("CCSInventoryManager::GetInstance() - Failed to get instance");
    } else {
        LOG_DEBUG("CCSInventoryManager::GetInstance() - Instance obtained successfully");
    }
    
    return instance;
}


CGCClientSharedObjectCache* CGCClient::FindSOCache(SOID_t ID,
    bool bCreateIfMissing) {
    using FnFindSOCache = CGCClientSharedObjectCache * (__fastcall*)(CGCClient*, SOID_t, bool);

    static FnFindSOCache oFindSOCache = reinterpret_cast<FnFindSOCache>(
        M::ResolveRelativeAddress(
            reinterpret_cast<uint8_t*>(M::FindPattern(L"client.dll", "E8 ? ? ? ? 48 8B F0 48 85 C0 74 0E 4C 8B C3"
            )),
            0x1,
            0x0
        )
        );

    return oFindSOCache(this, ID, bCreateIfMissing);
}


CGCClientSharedObjectTypeCache* CGCClientSharedObjectCache::CreateBaseTypeCache(int nClassID)
{
    using fnCreateBaseTypeCache = CGCClientSharedObjectTypeCache * (__thiscall*)(void*, int);
    static fnCreateBaseTypeCache oCreateBaseTypeCache = reinterpret_cast<fnCreateBaseTypeCache>( M::FindPattern(L"client.dll", "40 53 48 83 EC ? 4C 8B 49 ? 44 8B D2"));

    if (!oCreateBaseTypeCache)
        return nullptr;
    return oCreateBaseTypeCache(this, nClassID);

}

CGCClientSystem* CGCClientSystem::GetInstance() {
    LOG_DEBUG("CGCClientSystem::GetInstance() called");
    
    using FnGetInstanceSs = CGCClientSystem * (__fastcall*)();

    static FnGetInstanceSs fnGetClientSystem =
        reinterpret_cast<FnGetInstanceSs>(
            M::ResolveRelativeAddress(
                M::FindPattern(L"client.dll", "E8 ? ? ? ? 48 8B 4F 10 8B 1D ? ? ? ?"),
                0x1, 0x0
            )
        );

    if (!fnGetClientSystem) {
        LOG_ERROR("CGCClientSystem::GetInstance() - Failed to resolve function pattern");
        return nullptr;
    }

    auto instance = fnGetClientSystem();
    if (!instance) {
        LOG_ERROR("CGCClientSystem::GetInstance() - Failed to get instance");
    } else {
        LOG_DEBUG("CGCClientSystem::GetInstance() - Instance obtained successfully");
    }
    
    return instance;
}
CCSPlayerInventory* CCSPlayerInventory::GetInstance() {
    CCSInventoryManager* pInventoryManager = CCSInventoryManager::GetInstance();
    if (!pInventoryManager) return nullptr;

    return pInventoryManager->GetLocalInventory();
}

CGCClientSharedObjectTypeCache* CreateBaseTypeCache(
    CCSPlayerInventory* pInventory) {
    LOG_DEBUG("CreateBaseTypeCache() called");
    
    // VALIDAÇÃO: validate inventory pointer
    if (!pInventory) {
        LOG_ERROR("CreateBaseTypeCache: pInventory is null");
        return nullptr;
    }

    // Get SO Cache with validation (offset 0x68)
    CGCClientSharedObjectCache* SOCache = pInventory->GetSOCache();
    if (!SOCache) {
        LOG_ERROR("CreateBaseTypeCache: SOCache is null (offset 0x68 may be incorrect)");
        return nullptr;
    }
    
    LOG_DEBUG("CreateBaseTypeCache: SOCache obtained successfully");

    // Create base type cache (classID = 1 for CEconItem)
    CGCClientSharedObjectTypeCache* pTypeCache = SOCache->CreateBaseTypeCache(1);
    if (!pTypeCache) {
        LOG_ERROR("CreateBaseTypeCache: failed to create type cache for classID=1");
        return nullptr;
    }

    LOG_DEBUG("CreateBaseTypeCache: Type cache created successfully");
    return pTypeCache;
}

bool CCSPlayerInventory::AddEconItem(CEconItem* pItem) {
    LOG_INFO("=== CCSPlayerInventory::AddEconItem() called ===");
    
    // VALIDAÇÃO CRÍTICA: validate pItem pointer
    if (!pItem) {
        LOG_ERROR("AddEconItem: pItem is null - cannot add item");
        return false;
    }

    // VALIDAÇÃO CRÍTICA: validate inventory instance
    if (!this) {
        LOG_ERROR("AddEconItem: inventory instance (this) is null");
        return false;
    }

    // VALIDAÇÃO CRÍTICA: Verificar estrutura do item
    LOG_INFO("AddEconItem: Item details - ID: %llu, DefIndex: %d, AccountID: %u", 
             pItem->m_ulID, pItem->m_unDefIndex, pItem->m_unAccountID);
    
    // VALIDAÇÃO: Verificar se ID é válido
    if (pItem->m_ulID == 0) {
        LOG_WARNING("AddEconItem: Item ID is 0 - this may cause issues");
    }
    
    // Create type cache with validation
    LOG_DEBUG("AddEconItem: Creating base type cache...");
    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (!pSOTypeCache) {
        LOG_ERROR("AddEconItem: CRITICAL - failed to create SOTypeCache");
        return false;
    }

    // Try to add object to cache
    LOG_INFO("AddEconItem: Adding object to shared object cache...");
    if (!pSOTypeCache->AddObject((CSharedObject*)pItem)) {
        LOG_ERROR("AddEconItem: CRITICAL - failed to add object to cache");
        return false;
    }
    
    LOG_INFO("AddEconItem: Object added to cache successfully");

    // Notify system of new item (GetOwner() pode falhar com offset incorreto)
    LOG_DEBUG("AddEconItem: Getting owner information...");
    SOID_t owner = GetOwner();
    
    if (owner.m_id == 0 && owner.m_type == 0) {
        LOG_WARNING("AddEconItem: GetOwner returned invalid owner (ID=0, Type=0)");
        LOG_WARNING("AddEconItem: Offset 0x10 may be incorrect - item added to cache but may not persist");
        LOG_WARNING("AddEconItem: Skipping SOCreated notification to prevent crash");
        // Não chamar SOCreated se owner é inválido - isso pode prevenir o crash
    } else {
        LOG_INFO("AddEconItem: Owner valid (ID=%llu, Type=%u) - notifying system...", 
                 owner.m_id, owner.m_type);
        
        try {
            SOCreated(owner, (CSharedObject*)pItem, eSOCacheEvent_Incremental);
            LOG_INFO("AddEconItem: SOCreated notification sent successfully");
        } catch (...) {
            LOG_ERROR("AddEconItem: Exception caught during SOCreated - possible crash prevented");
            return false;
        }
    }
    
    LOG_SEPARATOR();
    LOG_INFO("SUCCESS: Item %llu added to inventory successfully", pItem->m_ulID);
    LOG_SEPARATOR();
    
    return true;
}
//
CEconItem* CCSPlayerInventory::GetSOCDataForItem(uint64_t itemID) {
    CEconItem* pSOCData = nullptr;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (pSOTypeCache) {
        const CUtlVector<CEconItem*>& vecItems =
            pSOTypeCache->GetVecObjects<CEconItem*>();

        for (CEconItem* it : vecItems) {
            if (it && it->m_ulID == itemID) {
                pSOCData = it;
                break;
            }
        }
    }

    return pSOCData;
}

void CCSPlayerInventory::RemoveEconItem(CEconItem* pItem) {
    // Helper function to aid in removing items.
    if (!pItem) return;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (!pSOTypeCache) return;

    const CUtlVector<CEconItem*>& pSharedObjects =
        pSOTypeCache->GetVecObjects<CEconItem*>();
    if (!pSharedObjects.Exists(pItem)) return;

    SODestroyed(GetOwner(), (CSharedObject*)pItem, eSOCacheEvent_Incremental);
    pSOTypeCache->RemoveObject((CSharedObject*)pItem);

    pItem->Destruct();
}

std::pair<uint64_t, uint32_t> CCSPlayerInventory::GetHighestIDs() {
    uint64_t maxItemID = 0;
    uint32_t maxInventoryID = 0;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (pSOTypeCache) {
        const CUtlVector<CEconItem*>& vecItems =
            pSOTypeCache->GetVecObjects<CEconItem*>();

        for (CEconItem* it : vecItems) {
            if (!it) continue;

            // Checks if item is default.
            if ((it->m_ulID & 0xF000000000000000) != 0) continue;

            maxItemID = std::max(maxItemID, it->m_ulID);
            maxInventoryID = std::max(maxInventoryID, it->m_unInventory);
        }
    }

    return std::make_pair(maxItemID, maxInventoryID);
}
