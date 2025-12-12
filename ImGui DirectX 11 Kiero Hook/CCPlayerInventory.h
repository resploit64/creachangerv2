#pragma once
#include <cstdio>
#include "logger.h"
#include "memory.h"
#include "utlvector.h"
class CEconItem;

class CCSPlayerInventory;
class CGCClientSharedObjectCache;
struct SOID_t {
    uint64_t m_id;
    uint32_t m_type;
    uint32_t m_padding;
};

class CGCClient {
public:
    CGCClientSharedObjectCache* FindSOCache(SOID_t ID,
        bool bCreateIfMissing = true);
};

class CGCClientSystem {
public:
    static CGCClientSystem* GetInstance();

    CGCClient* GetCGCClient() {
        return reinterpret_cast<CGCClient*>((uintptr_t)(this) + 0xB8);
    }
};


class CCSInventoryManager {
public:
    static CCSInventoryManager* GetInstance();

    auto EquipItemInLoadout(int iTeam, int iSlot, uint64_t iItemID) {
        return M::call_virtual<bool>(this, 65, iTeam, iSlot, iItemID);

    }

    auto GetLocalInventory() {
        return M::call_virtual<CCSPlayerInventory*>(this, 68);

    }
};
class CSharedObject;

class CGCClientSharedObjectTypeCache {
public:
    auto AddObject(CSharedObject* pObject) {
        return M::call_virtual<bool>(this, 1, pObject);
    }

    auto RemoveObject(CSharedObject* soIndex) {
        return M::call_virtual<CSharedObject*>(this, 3, soIndex);

    }

    template <typename T>
    auto& GetVecObjects() {
        // ATUALIZAÇÃO: Offset 0x8 validado - mas mantemos validação defensiva
        // NOTA: __try/__except não funciona com GCC, usando validação simples
        static CUtlVector<T> empty_vector;
        
        if (!this) {
            LOG_ERROR("GetVecObjects: this pointer is null");
            return empty_vector;
        }
        
        CUtlVector<T>* vec = reinterpret_cast<CUtlVector<T>*>((uintptr_t)(this) + 0x8);
        if (!vec || (uintptr_t)vec < 0x1000) {
            LOG_ERROR("GetVecObjects: vector pointer is invalid (0x%p) at offset 0x8", vec);
            return empty_vector;
        }
        
        return *vec;
    }
};

enum ESOCacheEvent {
    /// Dummy sentinel value
    eSOCacheEvent_None = 0,

    /// We received a our first update from the GC and are subscribed
    eSOCacheEvent_Subscribed = 1,

    /// We lost connection to GC or GC notified us that we are no longer
    /// subscribed. Objects stay in the cache, but we no longer receive updates
    eSOCacheEvent_Unsubscribed = 2,

    /// We received a full update from the GC on a cache for which we were
    /// already subscribed. This can happen if connectivity is lost, and then
    /// restored before we realized it was lost.
    eSOCacheEvent_Resubscribed = 3,

    /// We received an incremental update from the GC about specific object(s)
    /// being added, updated, or removed from the cache
    eSOCacheEvent_Incremental = 4,

    /// A lister was added to the cache
    /// @see CGCClientSharedObjectCache::AddListener
    eSOCacheEvent_ListenerAdded = 5,

    /// A lister was removed from the cache
    /// @see CGCClientSharedObjectCache::RemoveListener
    eSOCacheEvent_ListenerRemoved = 6,
};
class C_EconItemView;
class CCSPlayerInventory {
public:
    static CCSPlayerInventory* GetInstance();
    auto SOCreated(SOID_t owner, CSharedObject* pObject, ESOCacheEvent eEvent)
    {
        return M::call_virtual<void>(this, 0, owner, pObject, eEvent);
    }
    //auto SOCreated(SOID_t owner, CSharedObject* pObject, ESOCacheEvent eEvent) {
    //    return CALL_VIRTUAL(void, 0, this, owner, pObject, eEvent);
    //}

    //auto SOUpdated(SOID_t owner, CSharedObject* pObject, ESOCacheEvent eEvent) {
    //    return CALL_VIRTUAL(void, 1, this, owner, pObject, eEvent);
    //}

    CEconItem* GetSOCDataForItem(uint64_t itemID);


    auto SODestroyed(SOID_t owner, CSharedObject* pObject,
        ESOCacheEvent eEvent) {
        return M::call_virtual<void>(this, 2, owner, pObject, eEvent);
    }

    //auto GetItemInLoadout(int iClass, int iSlot) {
    //    return CALL_VIRTUAL(C_EconItemView*, 8, this, iClass, iSlot);
    //}
    auto GetItemInLoadout(int iClass, int iSlot) {
        return M::call_virtual<C_EconItemView*>(this, 8, iClass, iSlot);
    }

    bool AddEconItem(CEconItem* pItem);

    //bool AddEconItem(CEconItem* pItem);
    void RemoveEconItem(CEconItem* pItem);
    std::pair<uint64_t, uint32_t> GetHighestIDs();


    auto GetOwner() {
        // ATUALIZAÇÃO CRÍTICA: Adicionada validação para prevenir crashes
        // Offset 0x10 pode estar incorreto - aguardando pattern scanning
        // NOTA: __try/__except não funciona com GCC, usando validação simples
        
        if (!this) {
            LOG_ERROR("GetOwner: this pointer is null");
            return SOID_t{0, 0, 0};
        }
        
        SOID_t* owner = reinterpret_cast<SOID_t*>((uintptr_t)(this) + 0x10);
        
        // Validação básica de ponteiro (não perfeita, mas melhor que nada)
        if (!owner || (uintptr_t)owner < 0x1000) {
            LOG_ERROR("GetOwner: owner pointer is invalid (0x%p)", owner);
            return SOID_t{0, 0, 0};
        }
        
        // Retornar valor - pode crashar se offset estiver muito errado
        // mas logs ajudarão a identificar o problema
        return *owner;
    }
    
    CGCClientSharedObjectCache* GetSOCache() {
        // ATUALIZAÇÃO CRÍTICA: Adicionada validação para prevenir crashes
        // Offset 0x68 pode estar incorreto - aguardando pattern scanning
        // NOTA: __try/__except não funciona com GCC, usando validação simples
        
        if (!this) {
            LOG_ERROR("GetSOCache: this pointer is null");
            return nullptr;
        }
        
        CGCClientSharedObjectCache** cache_ptr = reinterpret_cast<CGCClientSharedObjectCache**>(
            reinterpret_cast<uint8_t*>(this) + 0x68);
        
        // Validação básica de ponteiro
        if (!cache_ptr || (uintptr_t)cache_ptr < 0x1000) {
            LOG_ERROR("GetSOCache: cache pointer is invalid (0x%p)", cache_ptr);
            return nullptr;
        }
        
        CGCClientSharedObjectCache* cache = *cache_ptr;
        if (!cache || (uintptr_t)cache < 0x1000) {
            LOG_ERROR("GetSOCache: cache is invalid (0x%p) - offset 0x68 may be incorrect", cache);
            return nullptr;
        }
        
        return cache;
    }
};





class CGCClientSharedObjectCache {
public:
    CGCClientSharedObjectTypeCache* CreateBaseTypeCache(int nClassID);
};

