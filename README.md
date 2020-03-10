# decomposition-viewer-jni
Native code for [decomposition-viewer](https://github.com/YaaZ/decomposition-viewer) application

## How to build
1. `git clone https://github.com/YaaZ/decomposition-viewer-jni.git`
2. `git submodule update --init --recursive`
3. Run Cmake

Cmake version is 3.15. You also need installed JDK 10 or newer and Vulkan SDK with its "Bin"
directory in your PATH (it needs "glslc" utility to compile shaders)
