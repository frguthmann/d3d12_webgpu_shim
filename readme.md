# D3D12 WebGPU Shim

This repository contains a shim of d3d12.dll which makes GPU profilers usable to inspect WebGPU workloads running on Chrome for Windows. If you would like to know more about why this thing exists and how it solves the problem at hand, please read my blog post <a href="https://frguthmann.github.io/posts/shimming_d3d12/" target="_blank" rel="noopener noreferrer">Shimming d3d12.dll for fun and profit</a>

## Disclaimer

> ⚠ **Before anything else, please remember that this is a hacky solution and that you should not browse the open web with that DLL attached. It should only be used for a GPU profiling session and removed afterwards. It also conflicts with PIX, so you should remove it before attempting to take a capture. I would recommend adding that DLL to a specific flavor of Chrome ( Dev, Canary, etc. ) that you would only use for GPU profiling.**

<a name="HOW_TO_USE"></a>

## How to use

A comprehensive explanation on how to use this dll is provided in the following article on my blog : <a href="https://frguthmann.github.io/posts/profiling_webgpu/" target="_blank" rel="noopener noreferrer">GPU profiling for WebGPU workloads on Windows with Chrome</a>

### TL;DR

1. Build or download `d3d12.dll` from this repository.
    + Place the DLL into the `\Google\Chrome\Application` folder.
2. On AMD, download `WinPixEventRuntime.dll` from the release section of <a href="https://github.com/frguthmann/PixEventsAMD" target="_blank" rel="noopener noreferrer">that repository</a>.
    + Place it into the `\Google\Chrome\Application\<Version Number>` folder.
3. When launching Chrome, use the following command line arguments:
    + `--no-sandbox --disable-gpu-sandbox --disable-gpu-watchdog --disable-direct-composition --enable-dawn-features=emit_hlsl_debug_symbols,disable_symbol_renaming`
4. Use your favorite GPU profiler as usual. 

## Building

```
git clone https://github.com/frguthmann/d3d12_webgpu_shim.git
cd d3d12_webgpu_shim
cmake ./
```

Then open the Visual Studio solution, build it and follow the [How to use](#HOW_TO_USE) instructions.

# Thanks

I would like to thank <a href="https://mastodon.gamedev.place/@theWarhelm" target="_blank" rel="noopener noreferrer">Marco Castorina</a> and <a href="https://x.com/raydey" target="_blank" rel="noopener noreferrer">Ray Dey</a> without whom this hack probably wouldn't exist :).