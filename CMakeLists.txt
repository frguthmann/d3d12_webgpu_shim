cmake_minimum_required(VERSION 3.0)

project (d3d12_webgpu_shim)

add_library(d3d12 SHARED
    src/d3d12_webgpu_shim.cpp
    src/d3d12_webgpu_shim.def
)