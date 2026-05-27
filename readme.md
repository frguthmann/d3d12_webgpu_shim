# D3D12 WebGPU Shim

This repository makes GPU profilers usable for inspecting WebGPU workloads running on Chrome for Windows. It does this through two complementary mechanisms: a legacy shim `d3d12.dll` that was placed next to Chrome but is now deprecated, and a Detours-based hook DLL injected at launch time by a dedicated launcher executable. If you would like to know more about why this thing exists and how it solves the problem at hand, please read my blog post <a href="https://frguthmann.github.io/posts/shimming_d3d12/" target="_blank" rel="noopener noreferrer">Shimming d3d12.dll for fun and profit</a>.

## Disclaimer

> ⚠ **Before anything else, please remember that this is a hacky solution and that you should not browse the open web with these tools attached. They should only be used for a GPU profiling session.**

<a name="HOW_TO_USE"></a>

## How it works

The repository builds four artifacts:

| Artifact | Description |
|---|---|
| `d3d12.dll` | Legacy shim DLL. Placed next to `chrome.exe`, it intercepts D3D12 calls before Chrome can load the real `d3d12.dll`. |
| `d3d12_webgpu_hook.dll` | Hook DLL that uses [Microsoft Detours](https://github.com/microsoft/Detours) to patch D3D12 entry points in-process at runtime. This avoids touching Chrome's install directory. |
| `webgpu_profiling_launcher.exe` | Launcher that creates Chrome suspended, injects `d3d12_webgpu_hook.dll` via `CreateRemoteThread` + `LoadLibrary`, then resumes the process. |
| `webgpu_injector.exe` | Standalone injector that attaches `d3d12_webgpu_hook.dll` to an already-running process by PID or process name. Use this when your profiler (e.g. Nsight) needs to be the one launching Chrome. |

The launcher approach is now the default one as Chrome has hardened its DLL loading policies and prevents loading our custom `d3d12.dll` by default.

## How to use

A comprehensive explanation on how to use these tools is provided in the following article on my blog : <a href="https://frguthmann.github.io/posts/profiling_webgpu/" target="_blank" rel="noopener noreferrer">GPU profiling for WebGPU workloads on Windows with Chrome</a>.

### Option A — Launcher (recommended)

1. Build the project (see [Building](#building)) or download the artifacts from the releases page.
2. On AMD, download `WinPixEventRuntime.dll` from the release section of <a href="https://github.com/frguthmann/PixEventsAMD" target="_blank" rel="noopener noreferrer">that repository</a> and place it in the `<Chrome Dir>\<Version Number>` folder.
3. Run the launcher, passing the path to `chrome.exe` and any extra Chrome flags. Ex with recommended flags:
    ```
    webgpu_profiling_launcher.exe [path\to\chrome.exe] --no-sandbox --disable-gpu-sandbox --disable-gpu-watchdog --disable-direct-composition --do-not-de-elevate --enable-dawn-features=emit_hlsl_debug_symbols,disable_symbol_renaming
    ```
4. The launcher will start Chrome with the hook already active. Use your favourite GPU profiler as usual.

### Option B — Injector (for profilers that control the launch, e.g. Nsight)

Some GPU profilers such as NVIDIA Nsight need to launch the target process themselves in order to instrument it. In that case, use `webgpu_injector.exe` to attach the hook DLL after Chrome is already running.

1. Build the project or download the artifacts from the releases page.
2. On AMD, place `WinPixEventRuntime.dll` in the `<Chrome Dir>\<Version Number>` folder (see Option A step 2).
3. Launch Chrome through your profiler as usual, passing the recommended flags:
    ```
    --no-sandbox --disable-gpu-sandbox --disable-gpu-watchdog --disable-direct-composition --do-not-de-elevate --enable-dawn-features=emit_hlsl_debug_symbols,disable_symbol_renaming
    ```
4. Once Chrome is running, run the injector, targeting all Chrome processes by name:
    ```
    webgpu_injector.exe chrome.exe
    ```
    Windows will prompt for elevation automatically (UAC). This injects the hook into every running `chrome.exe` process. In the browser process the hook patches `CreateProcessW` so that any GPU child process Chrome spawns afterwards also receives the DLL automatically. If the GPU process is already running, it is patched directly as well.
5. Reload the tab with the WebGPU workload.
6. Use your profiler as usual.

### Option C (DEPRECATED) — Legacy shim DLL

1. Build or download `d3d12.dll` from this repository.
    + Place the DLL into the `\Google\Chrome\Application` folder.
2. On AMD, download `WinPixEventRuntime.dll` from the release section of <a href="https://github.com/frguthmann/PixEventsAMD" target="_blank" rel="noopener noreferrer">that repository</a>.
    + Place it into the `\Google\Chrome\Application\<Version Number>` folder.
3. When launching Chrome, use the following command line arguments:
    + `--no-sandbox --disable-gpu-sandbox --disable-gpu-watchdog --disable-direct-composition --do-not-de-elevate --enable-dawn-features=emit_hlsl_debug_symbols,disable_symbol_renaming`
4. Use your favorite GPU profiler as usual.

<a name="building"></a>

## Building

Prerequisites: Visual Studio 2019 or later with the **Desktop development with C++** workload, and CMake 3.14+.

[Microsoft Detours](https://github.com/microsoft/Detours) is fetched and built automatically by CMake — no manual setup is required.

```
git clone https://github.com/frguthmann/d3d12_webgpu_shim.git
cd d3d12_webgpu_shim
cmake -S . -B build
cmake --build build --config RelWithDebInfo
```

The four artifacts are placed in `build\RelWithDebInfo\`. Alternatively, open the generated Visual Studio solution (`build\d3d12_webgpu_shim.sln`) and build from the IDE.

Then follow the [How to use](#HOW_TO_USE) instructions.

# Thanks

I would like to thank <a href="https://mastodon.gamedev.place/@theWarhelm" target="_blank" rel="noopener noreferrer">Marco Castorina</a> and <a href="https://x.com/raydey" target="_blank" rel="noopener noreferrer">Ray Dey</a> without whom this hack probably wouldn't exist :).
