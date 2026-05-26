cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0091 NEW)

project(d3d12_webgpu_shim)

include(FetchContent)
FetchContent_Declare(
    detours
    GIT_REPOSITORY https://github.com/microsoft/Detours.git
    GIT_TAG        v4.0.1
)
FetchContent_MakeAvailable(detours)

# Build Detours as a static library
add_library(detours STATIC
    ${detours_SOURCE_DIR}/src/detours.cpp
    ${detours_SOURCE_DIR}/src/modules.cpp
    ${detours_SOURCE_DIR}/src/disasm.cpp
    ${detours_SOURCE_DIR}/src/image.cpp
    ${detours_SOURCE_DIR}/src/creatwth.cpp
)
target_include_directories(detours PUBLIC ${detours_SOURCE_DIR}/src)
target_compile_definitions(detours PRIVATE WIN32_LEAN_AND_MEAN)

# --- Original shim DLL (legacy, for systems where DLL placement still works) ---
add_library(d3d12 SHARED
    src/d3d12_webgpu_shim.cpp
    src/d3d12_webgpu_shim.def
)

# --- Hook DLL (injected via Detours, hooks the real d3d12.dll in-process) ---
add_library(d3d12_webgpu_hook SHARED
    src/dll_injection.cpp
    src/d3d12_webgpu_hook.cpp
)
# Only link Detours statically. D3D12/DXGI are resolved at runtime via GetProcAddress
# to avoid loader conflicts when injected into Chrome.
# Use static CRT to avoid VCRUNTIME DLL search path issues in Chrome's process.
target_link_libraries(d3d12_webgpu_hook PRIVATE detours)
set_property(TARGET d3d12_webgpu_hook PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET detours PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- Launcher exe (spawns Chrome suspended, injects hook DLL via CreateRemoteThread) ---
add_executable(webgpu_profiling_launcher
    src/dll_injection.cpp
    src/launcher.cpp
)