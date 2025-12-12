================================================================================
                         HOOK MANAGER - РУКОВОДСТВО
================================================================================

ОПИСАНИЕ:
---------
Менеджер хуков предоставляет удобный интерфейс для создания и управления 
множеством хуков с использованием MinHook (MH_CreateHook).

ФАЙЛЫ:
------
- hook_manager.h   - Заголовочный файл с классом CBaseHookObject и namespace H
- hook_manager.cpp - Реализация функций Setup() и Destroy()
- hook_example.h   - Примеры использования (можно удалить после изучения)

================================================================================
                        КАК ИСПОЛЬЗОВАТЬ
================================================================================

ШАГ 1: ОБЪЯВЛЕНИЕ ТИПОВ ФУНКЦИЙ
--------------------------------
Определите типы функций, которые вы хотите хукнуть:

typedef void(__fastcall* DrawObject_t)(void* thisptr, int a1, int a2);
typedef float(__fastcall* GetFOV_t)(void* pCameraServices);


ШАГ 2: ОБЪЯВЛЕНИЕ ХУКОВ В hook_manager.h
-----------------------------------------
В namespace H добавьте ваши хуки:

namespace H
{
    bool Setup();
    void Destroy();

    inline CBaseHookObject<DrawObject_t> hkDrawObject = {};
    inline CBaseHookObject<GetFOV_t> hkGetFOV = {};
}


ШАГ 3: СОЗДАНИЕ DETOUR ФУНКЦИЙ
-------------------------------
Создайте функции-обертки (detours), которые будут вызваны вместо оригинальных:

void __fastcall DrawObject_Detour(void* thisptr, int a1, int a2)
{
    // Ваш код ДО вызова оригинальной функции
    printf("[*] DrawObject called!\n");

    // Вызов оригинальной функции
    H::hkDrawObject.GetOriginal()(thisptr, a1, a2);

    // Ваш код ПОСЛЕ вызова оригинальной функции
}

float __fastcall GetFOV_Detour(void* pCameraServices)
{
    // Вызываем оригинал
    float originalFOV = H::hkGetFOV.GetOriginal()(pCameraServices);
    
    // Изменяем значение
    return originalFOV + 10.0f;
}


ШАГ 4: ИНИЦИАЛИЗАЦИЯ ХУКОВ В H::Setup()
----------------------------------------
В файле hook_manager.cpp, в функции H::Setup() добавьте создание хуков:

bool H::Setup()
{
    if (MH_Initialize() != MH_OK)
        return false;

    // Получить адреса функций (через сигнатуры, паттерны, офсеты и т.д.)
    void* pDrawObject = reinterpret_cast<void*>(0x12345678);
    void* pGetFOV = reinterpret_cast<void*>(0x87654321);

    // Создать хуки
    if (!hkDrawObject.Create(pDrawObject, &DrawObject_Detour))
    {
        printf("[!] Failed to create DrawObject hook\n");
        return false;
    }

    if (!hkGetFOV.Create(pGetFOV, &GetFOV_Detour))
    {
        printf("[!] Failed to create GetFOV hook\n");
        return false;
    }

    printf("[+] All hooks created successfully\n");
    return true;
}


ШАГ 5: ОЧИСТКА ХУКОВ В H::Destroy()
------------------------------------
В файле hook_manager.cpp, в функции H::Destroy() добавьте удаление хуков:

void H::Destroy()
{
    if (hkDrawObject.IsHooked())
        hkDrawObject.Remove();
    
    if (hkGetFOV.IsHooked())
        hkGetFOV.Remove();

    MH_Uninitialize();
}


ШАГ 6: ВЫЗОВ В MAIN
--------------------
В вашем main.cpp вызовите Setup() и Destroy():

DWORD WINAPI MainThread(LPVOID lpReserved)
{
    // Инициализация
    if (!H::Setup())
    {
        printf("[!] Failed to setup hooks\n");
        return FALSE;
    }

    // Ваш основной код...
    while (!shouldExit)
    {
        Sleep(100);
    }

    // Очистка
    H::Destroy();
    
    return TRUE;
}

================================================================================
                        МЕТОДЫ CBaseHookObject
================================================================================

Create(void* pFunction, void* pDetour)
    - Создает хук для функции pFunction с detour pDetour
    - Автоматически включает хук после создания
    - Возвращает: true если успешно, false если ошибка

Replace()
    - Включает ранее созданный хук
    - Возвращает: true если успешно, false если ошибка

Restore()
    - Временно отключает хук (можно включить обратно с Replace())
    - Возвращает: true если успешно, false если ошибка

Remove()
    - Полностью удаляет хук (нельзя включить обратно)
    - Возвращает: true если успешно, false если ошибка

GetOriginal()
    - Возвращает указатель на оригинальную функцию
    - Используйте для вызова оригинала из detour функции

IsHooked()
    - Возвращает true если хук активен, false если нет

================================================================================
                          ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ
================================================================================

ПРИМЕР 1: ПРОСТОЙ ХУК С ЛОГИРОВАНИЕМ
-------------------------------------
void __fastcall MyFunction_Detour(int param)
{
    printf("MyFunction called with param: %d\n", param);
    H::hkMyFunction.GetOriginal()(param);
}


ПРИМЕР 2: ИЗМЕНЕНИЕ ВОЗВРАЩАЕМОГО ЗНАЧЕНИЯ
-------------------------------------------
bool __fastcall IsVisible_Detour(void* entity)
{
    // Всегда возвращаем true (делаем все видимым)
    return true;
    
    // Или вызываем оригинал:
    // return H::hkIsVisible.GetOriginal()(entity);
}


ПРИМЕР 3: ИЗМЕНЕНИЕ ПАРАМЕТРОВ
-------------------------------
void __fastcall SetHealth_Detour(void* player, float health)
{
    // Ограничиваем здоровье максимумом 100
    if (health > 100.0f)
        health = 100.0f;
    
    // Вызываем оригинал с измененным параметром
    H::hkSetHealth.GetOriginal()(player, health);
}


ПРИМЕР 4: ВРЕМЕННОЕ ОТКЛЮЧЕНИЕ ХУКА
------------------------------------
void DoSomethingWithoutHook()
{
    // Отключаем хук
    if (H::hkMyFunction.IsHooked())
        H::hkMyFunction.Restore();
    
    // Делаем что-то...
    SomeOperation();
    
    // Включаем обратно
    H::hkMyFunction.Replace();
}

================================================================================
                            СОВЕТЫ И ПОДСКАЗКИ
================================================================================

1. Всегда проверяйте возвращаемые значения Create() и Setup()
2. Вызывайте Destroy() перед выгрузкой DLL
3. Используйте GetOriginal() для вызова оригинальной функции из detour
4. Сигнатуры функций (calling convention, параметры) должны точно совпадать
5. Не забывайте про calling convention (__fastcall, __stdcall, __cdecl и т.д.)
6. При ошибках проверьте адреса функций и сигнатуры

================================================================================



