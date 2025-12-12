# Руководство по правильной выгрузке DLL

## Проблема
При выгрузке DLL с DirectX хуками и ImGui интерфейсом возникают проблемы:
- DirectX ресурсы не освобождаются корректно
- ImGui контекст не очищается
- Kiero хуки остаются активными
- Может произойти краш приложения

## Решение

### 1. Правильная последовательность выгрузки

```cpp
void Unload() {
    // 1. Установить флаг выгрузки
    SDK::shouldUnload = true;
    
    // 2. Подождать завершения очистки DirectX
    Sleep(100);
    
    // 3. Уничтожить MinHook хуки
    H::Destroy();
    
    // 4. Закрыть логгер
    logger::shutdown();
    
    // 5. Отключить Kiero хуки
    kiero::shutdown();
    
    // 6. Выгрузить DLL
    FreeLibraryAndExitThread(g_hModule, 0);
}
```

### 2. Очистка DirectX ресурсов в hkPresent

```cpp
if (SDK::shouldUnload) {
    // 1. Очистить ImGui
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    // 2. Восстановить оригинальный WndProc
    SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
    
    // 3. Освободить DirectX ресурсы
    CleanupRenderTarget();
    pDevice->Release();
    pContext->Release();
    
    // 4. Вызвать оригинальный Present
    HRESULT result = oPresent(pSwapChain, SyncInterval, Flags);
    
    // 5. Отключить Kiero хуки
    kiero::unbind(8);  // Present
    kiero::unbind(13); // ResizeBuffers
    
    return result;
}
```

### 3. Способы выгрузки DLL

#### Способ 1: Горячая клавиша
Нажмите `END` для безопасной выгрузки DLL.

#### Способ 2: Программно
```cpp
// Из внешнего кода
RequestUnload();
```

#### Способ 3: Автоматически
DLL автоматически выгрузится при закрытии процесса.

### 4. Важные моменты

1. **Никогда не вызывайте `Unload()` напрямую** - используйте `RequestUnload()`
2. **Всегда ждите завершения очистки DirectX** перед продолжением
3. **Проверяйте указатели на nullptr** перед освобождением
4. **Восстанавливайте оригинальные функции** перед отключением хуков

### 5. Отладка

Для отладки проблем с выгрузкой добавьте логирование:

```cpp
printf("[+] Starting DLL unload sequence...\n");
printf("[-] Unloading DirectX resources...\n");
printf("[+] DLL unload sequence completed\n");
```

### 6. Проверка успешной выгрузки

После выгрузки проверьте:
- Нет ли утечек памяти
- Не остались ли активные хуки
- Корректно ли работает приложение
- Нет ли ошибок в логах

## Пример использования

```cpp
// В вашем коде
if (some_condition) {
    RequestUnload(); // Безопасная выгрузка
}
```

Этот подход гарантирует полную и безопасную выгрузку DLL без крашей и утечек памяти.



