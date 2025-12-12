#include "includes.h"
#include "logger.h"
#include "inventory_p.h"
#include "skins.h"
#include "SchemaSystem.h"

void Unload() {
        LOG_INFO("=== Starting DLL unload sequence ===");
        
        // Set unload flag to trigger DirectX cleanup
        SDK::shouldUnload = true;
        
        // Wait a bit for DirectX cleanup to complete
        Sleep(100);
        
        LOG_INFO("Shutting down skins system...");
        S::Shutdown();
        
        LOG_INFO("Destroying hooks...");
        H::Destroy();
        
        LOG_INFO("Shutting down kiero...");
        kiero::shutdown();
        
        LOG_INFO("Clearing SchemaSystem cache...");
        SchemaSystem::Clear();
        
        // Shutdown logger (must be last)
        LOG_INFO("=== DLL unload complete ===");
        CreatinaLogger::Logger::Shutdown();
        
        // Exit thread and unload DLL
        FreeLibraryAndExitThread(g_hModule, 0);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
        // INICIALIZAR LOGGER PRIMEIRO
        CreatinaLogger::Logger::Initialize(CreatinaLogger::LogLevel::DEBUG);
        
        LOG_SEPARATOR();
        LOG_INFO("=== creatinawtf.dll Main Thread Started ===");
        LOG_INFO("DLL Module Handle: 0x%p", g_hModule);
        LOG_SEPARATOR();
        
        bool init_hook = false;
        bool modules_loaded = false;

        LOG_INFO("Waiting for matchmaking.dll to load...");
        while (GetModuleHandle(L"matchmaking.dll") == nullptr) {
                Sleep(100);
        }
        LOG_INFO("matchmaking.dll loaded successfully");
        
        while (!modules_loaded) {
                LOG_INFO("Initializing interfaces...");
                if (!I::initialize()) {
                        LOG_ERROR("Failed to initialize interfaces!");
                        Sleep(1000);
                        continue;
                }
                LOG_INFO("Interfaces initialized successfully");
                
                LOG_INFO("Initializing SchemaSystem...");
                if (!SchemaSystem::Setup(L"schemasystem.dll")) {
                        LOG_ERROR("Failed to initialize SchemaSystem!");
                        LOG_WARNING("Continuing with fallback hardcoded offsets...");
                } else {
                        LOG_INFO("SchemaSystem initialized successfully with %zu offsets", SchemaSystem::m_mapSchemaOffsets.size());
                }
                
                LOG_INFO("Setting up hooks...");
                if (!H::Setup()) {
                        LOG_ERROR("Failed to initialize hooks!");
                        Sleep(1000);
                        continue;
                }
                LOG_INFO("Hooks initialized successfully");

                modules_loaded = true;
        }
        
        LOG_INFO("Initializing Kiero DirectX hooks...");
        do {
                if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
                {
                        LOG_INFO("Kiero initialized, binding hooks...");
                        kiero::bind(8, (void**)&oPresent, (void*)hkPresent);
                        kiero::bind(13, (void**)&oResizeBuffers, (void*)hkResizeBuffers);
                        init_hook = true;
                        LOG_INFO("DirectX hooks bound successfully");
                }
                else {
                        LOG_WARNING("Kiero init failed, retrying...");
                        Sleep(100);
                }
        } while (!init_hook);
        
        LOG_SEPARATOR();
        LOG_INFO("=== DLL Initialization Complete ===");
        LOG_SEPARATOR();
        
        static bool added = false;

        while (!SDK::shouldUnload) {
                Sleep(500);
                
                if (!Events->inited) {
                        if (Events->Intilization()) {
                                LOG_INFO("Event manager initialized successfully");
                        }
                }
                
                if (InventoryPersistence::init() && !added) {
                        LOG_INFO("Loading persisted items from JSON...");
                        int restored = InventoryPersistence::LoadAndRecreate("added_items.json");
                        if (restored > 0) {
                                LOG_INFO("Successfully restored %d items from JSON", restored);
                                added = true;
                                InventoryPersistence::Equip();
                        }
                        else {
                                LOG_WARNING("No items restored from JSON (file may not exist or be empty)");
                        }
                }

                if (INVENTORY::Dump()) {
                        // Log already done in INVENTORY::Dump()
                }
        }

        Unload();
        return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
        switch (dwReason)
        {
        case DLL_PROCESS_ATTACH:
                g_hModule = hMod;
                DisableThreadLibraryCalls(hMod);
                CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
                break;
        case DLL_PROCESS_DETACH:
                // Only call Unload if we haven't already unloaded
                if (!SDK::shouldUnload) {
                        Unload();
                }
                break;
        }
        return TRUE;
}