/*
 * ПОЛНЫЙ ПРИМЕР ИНТЕГРАЦИИ HOOK MANAGER
 * 
 * Этот файл показывает как интегрировать hook manager в ваш проект
 * Вы можете использовать этот код как шаблон
 */

#include "hook_manager.h"
#include "hook_macros.h"
#include <cstdio>

// ============================================
// СПОСОБ 1: ИСПОЛЬЗОВАНИЕ БЕЗ МАКРОСОВ
// ============================================

// Определяем типы функций
typedef void(__fastcall* DrawObject_t)(void* thisptr, int a1, int a2);
typedef float(__fastcall* GetFOV_t)(void* pCameraServices);
typedef bool(__stdcall* IsVisible_t)(void* entity);

// Добавляем в namespace H (в реальном проекте добавьте в hook_manager.h)
// namespace H
// {
//     inline CBaseHookObject<DrawObject_t> hkDrawObject = {};
//     inline CBaseHookObject<GetFOV_t> hkGetFOV = {};
//     inline CBaseHookObject<IsVisible_t> hkIsVisible = {};
// }

// Detour функции
void __fastcall DrawObject_Detour(void* thisptr, int a1, int a2)
{
    printf("[DrawObject] Called with a1=%d, a2=%d\n", a1, a2);
    
    // Вызов оригинальной функции
    // H::hkDrawObject.GetOriginal()(thisptr, a1, a2);
    
    printf("[DrawObject] Finished\n");
}

float __fastcall GetFOV_Detour(void* pCameraServices)
{
    // Получаем оригинальный FOV
    // float originalFOV = H::hkGetFOV.GetOriginal()(pCameraServices);
    float originalFOV = 90.0f; // Пример
    
    printf("[GetFOV] Original: %.2f, Modified: %.2f\n", originalFOV, originalFOV + 20.0f);
    
    // Возвращаем измененный FOV
    return originalFOV + 20.0f;
}

bool __stdcall IsVisible_Detour(void* entity)
{
    // Делаем все сущности видимыми
    printf("[IsVisible] Entity: %p - Force visible\n", entity);
    return true;
    
    // Или вызываем оригинал:
    // return H::hkIsVisible.GetOriginal()(entity);
}

// ============================================
// СПОСОБ 2: ИСПОЛЬЗОВАНИЕ С МАКРОСАМИ
// ============================================

// Объявляем хуки с помощью макросов
DECLARE_HOOK(SetHealth, void, __fastcall, void* player, float health)
DECLARE_HOOK(GetPosition, void*, __stdcall, int entityId)

// Detour с макросами
DETOUR_DECL(SetHealth, void, __fastcall, void* player, float health)
{
    printf("[SetHealth] Player: %p, Health: %.2f\n", player, health);
    
    // Ограничиваем максимальное здоровье
    if (health > 100.0f)
        health = 100.0f;
    
    // Вызываем оригинал
    CALL_ORIGINAL(SetHealth, player, health);
}

DETOUR_DECL(GetPosition, void*, __stdcall, int entityId)
{
    printf("[GetPosition] EntityId: %d\n", entityId);
    
    // Вызываем оригинал
    return CALL_ORIGINAL(GetPosition, entityId);
}

// ============================================
// НАСТРОЙКА ХУКОВ
// ============================================

namespace H
{
    bool Setup()
    {
        printf("[*] Initializing Hook Manager...\n");
        
        // Инициализация MinHook
        if (const MH_STATUS status = MH_Initialize(); status != MH_OK)
        {
            printf("[!] Failed to initialize MinHook: %s\n", MH_StatusToString(status));
            return false;
        }
        printf("[+] MinHook initialized\n");

        // ========================================
        // СОЗДАНИЕ ХУКОВ - СПОСОБ 1 (БЕЗ МАКРОСОВ)
        // ========================================
        
        /*
        // Получаем адреса функций (замените на реальные адреса)
        void* pDrawObject = reinterpret_cast<void*>(0x12345678);
        void* pGetFOV = reinterpret_cast<void*>(0x87654321);
        void* pIsVisible = reinterpret_cast<void*>(0xABCDEF01);

        // Создаем хуки
        if (!hkDrawObject.Create(pDrawObject, &DrawObject_Detour))
        {
            printf("[!] Failed to create DrawObject hook\n");
            return false;
        }
        printf("[+] DrawObject hook created\n");

        if (!hkGetFOV.Create(pGetFOV, &GetFOV_Detour))
        {
            printf("[!] Failed to create GetFOV hook\n");
            return false;
        }
        printf("[+] GetFOV hook created\n");

        if (!hkIsVisible.Create(pIsVisible, &IsVisible_Detour))
        {
            printf("[!] Failed to create IsVisible hook\n");
            return false;
        }
        printf("[+] IsVisible hook created\n");
        */

        // ========================================
        // СОЗДАНИЕ ХУКОВ - СПОСОБ 2 (С МАКРОСАМИ)
        // ========================================
        
        /*
        CREATE_HOOK(SetHealth, 0x11111111)
        CREATE_HOOK(GetPosition, 0x22222222)
        */

        printf("[+] All hooks created successfully\n");
        return true;
    }

    void Destroy()
    {
        printf("[*] Destroying Hook Manager...\n");

        // ========================================
        // УДАЛЕНИЕ ХУКОВ - СПОСОБ 1 (БЕЗ МАКРОСОВ)
        // ========================================
        
        /*
        if (hkDrawObject.IsHooked())
        {
            hkDrawObject.Remove();
            printf("[+] DrawObject hook removed\n");
        }

        if (hkGetFOV.IsHooked())
        {
            hkGetFOV.Remove();
            printf("[+] GetFOV hook removed\n");
        }

        if (hkIsVisible.IsHooked())
        {
            hkIsVisible.Remove();
            printf("[+] IsVisible hook removed\n");
        }
        */

        // ========================================
        // УДАЛЕНИЕ ХУКОВ - СПОСОБ 2 (С МАКРОСАМИ)
        // ========================================
        
        /*
        REMOVE_HOOK(SetHealth)
        REMOVE_HOOK(GetPosition)
        */

        // Деинициализация MinHook
        if (const MH_STATUS status = MH_Uninitialize(); status != MH_OK)
        {
            printf("[!] Failed to uninitialize MinHook: %s\n", MH_StatusToString(status));
            return;
        }

        printf("[+] Hook Manager destroyed successfully\n");
    }
}

// ============================================
// ИНТЕГРАЦИЯ В MAIN
// ============================================

/*

// В вашем main.cpp:

DWORD WINAPI MainThread(LPVOID lpReserved)
{
    logger::initialize();

    // Ждем загрузки модулей
    while (GetModuleHandle("matchmaking.dll") == nullptr)
    {
        Sleep(100);
    }

    // Инициализируем интерфейсы
    if (!I::initialize())
    {
        printf("[!] Failed to initialize interfaces!\n");
        return FALSE;
    }

    // === ИНИЦИАЛИЗАЦИЯ HOOK MANAGER ===
    if (!H::Setup())
    {
        printf("[!] Failed to setup Hook Manager!\n");
        return FALSE;
    }
    printf("[+] Hook Manager initialized\n");

    // Инициализация Kiero (DirectX хуки)
    bool init_hook = false;
    do {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
        {
            kiero::bind(8, (void**)&oPresent, hkPresent);
            kiero::bind(13, (void**)&oResizeBuffers, hkResizeBuffers);
            init_hook = true;
        }
    } while (!init_hook);

    // Основной цикл
    while (!SDK::shouldUnload)
    {
        Sleep(1000);
    }

    // === ОЧИСТКА HOOK MANAGER ===
    printf("[*] Shutting down...\n");
    H::Destroy();

    // Остальная очистка
    logger::shutdown();
    kiero::shutdown();
    
    FreeLibraryAndExitThread(g_hModule, 0);
    return TRUE;
}

*/

// ============================================
// ДОПОЛНИТЕЛЬНЫЕ УТИЛИТЫ
// ============================================

namespace HookUtils
{
    // Функция для временного отключения хука
    template<typename T>
    class ScopedHookDisable
    {
    public:
        explicit ScopedHookDisable(CBaseHookObject<T>& hook) : m_Hook(hook), m_WasHooked(false)
        {
            if (m_Hook.IsHooked())
            {
                m_Hook.Restore();
                m_WasHooked = true;
            }
        }

        ~ScopedHookDisable()
        {
            if (m_WasHooked)
            {
                m_Hook.Replace();
            }
        }

    private:
        CBaseHookObject<T>& m_Hook;
        bool m_WasHooked;
    };

    // Использование:
    /*
    void DoSomethingWithoutHook()
    {
        // Хук автоматически отключится в этой области и включится при выходе
        ScopedHookDisable<DrawObject_t> disable(H::hkDrawObject);
        
        // Делаем что-то без хука...
        SomeOperation();
        
    } // Хук автоматически включится здесь
    */
}



