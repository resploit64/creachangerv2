cmake_minimum_required(VERSION 3.15)
project(creatinav2)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output to DLL
add_library(creatinav2 SHARED
    # Main files
    "ImGui DirectX 11 Kiero Hook/main.cpp"
    "ImGui DirectX 11 Kiero Hook/CCPlayerInventory.cpp"
    "ImGui DirectX 11 Kiero Hook/CEconItem.cpp"
    "ImGui DirectX 11 Kiero Hook/directx.cpp"
    "ImGui DirectX 11 Kiero Hook/draw.cpp"
    "ImGui DirectX 11 Kiero Hook/entity.cpp"
    "ImGui DirectX 11 Kiero Hook/events.cpp"
    "ImGui DirectX 11 Kiero Hook/fonts.cpp"
    "ImGui DirectX 11 Kiero Hook/hook_manager.cpp"
    "ImGui DirectX 11 Kiero Hook/interfaces.cpp"
    "ImGui DirectX 11 Kiero Hook/inventory.cpp"
    "ImGui DirectX 11 Kiero Hook/inventory_p.cpp"
    "ImGui DirectX 11 Kiero Hook/logger.cpp"
    "ImGui DirectX 11 Kiero Hook/memory.cpp"
    "ImGui DirectX 11 Kiero Hook/menu.cpp"
    "ImGui DirectX 11 Kiero Hook/schema.cpp"
    "ImGui DirectX 11 Kiero Hook/SchemaSystem.cpp"
    "ImGui DirectX 11 Kiero Hook/skins.cpp"
    "ImGui DirectX 11 Kiero Hook/panorama.cpp"
    "ImGui DirectX 11 Kiero Hook/widgets.cpp"
    
    # ImGui files
    "ImGui DirectX 11 Kiero Hook/imgui/imgui.cpp"
    "ImGui DirectX 11 Kiero Hook/imgui/imgui_demo.cpp"
    "ImGui DirectX 11 Kiero Hook/imgui/imgui_draw.cpp"
    "ImGui DirectX 11 Kiero Hook/imgui/imgui_impl_dx11.cpp"
    "ImGui DirectX 11 Kiero Hook/imgui/imgui_impl_win32.cpp"
    "ImGui DirectX 11 Kiero Hook/imgui/imgui_tables.cpp"
    "ImGui DirectX 11 Kiero Hook/imgui/imgui_widgets.cpp"
    
    # Kiero files
    "ImGui DirectX 11 Kiero Hook/kiero/kiero.cpp"
    
    # MinHook files
    "ImGui DirectX 11 Kiero Hook/kiero/minhook/src/buffer.c"
    "ImGui DirectX 11 Kiero Hook/kiero/minhook/src/hde/hde32.c"
    "ImGui DirectX 11 Kiero Hook/kiero/minhook/src/hde/hde64.c"
    "ImGui DirectX 11 Kiero Hook/kiero/minhook/src/hook.c"
    "ImGui DirectX 11 Kiero Hook/kiero/minhook/src/trampoline.c"
)

# Include directories
target_include_directories(creatinav2 PRIVATE
    "${CMAKE_SOURCE_DIR}/ImGui DirectX 11 Kiero Hook"
    "${CMAKE_SOURCE_DIR}/ImGui DirectX 11 Kiero Hook/imgui"
    "${CMAKE_SOURCE_DIR}/ImGui DirectX 11 Kiero Hook/kiero"
    "${CMAKE_SOURCE_DIR}/ImGui DirectX 11 Kiero Hook/kiero/minhook/include"
    "${CMAKE_SOURCE_DIR}/ImGui DirectX 11 Kiero Hook/kiero/minhook/src"
)

# Compiler flags
target_compile_options(creatinav2 PRIVATE
    -Wall
    -Wno-unknown-pragmas
    -static-libgcc
    -static-libstdc++
    -municode
)

# Linker flags
target_link_options(creatinav2 PRIVATE
    -Wl,--allow-multiple-definition
)

target_link_libraries(creatinav2 PRIVATE
    d3d11
    dxgi
    d3dcompiler
    user32
    gdi32
    kernel32
    imm32
    dwmapi
    -static
)

# Set DLL properties
set_target_properties(creatinav2 PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# UTF-8 encoding
add_compile_definitions(UNICODE _UNICODE)
