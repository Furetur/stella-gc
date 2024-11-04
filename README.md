# GC for Stella

## Build instructions

### Simplest example

1. Configure CMake: `cmake -B build`
2. Build with CMake: `cmake --build build`

This will generate `build/libstella_gc.a` for the GC (without runtime) and `build/libstella_runtime.a` for the Stella runtime library.

You can use the GC object file independently -- just link it with your own runtime and have fun!

### Build with stella programs

`-DBUILD_STELLA_PROGRAMS=ON -DSTELLA_COMPILER=/path/to/stella/compiler`

If you add these two arguments to the "Configure CMake" command, CMake will also build Stella programs from the `examples/` directory. It will statically link them with the runtime and GC. Built binaries will be located under `build/stella_examples`.

### Additional development options

* `-DSTRICT_BUILD_MODE=ON|OFF` Build the GC source files with `-Wall -Wpedantic ...`
* `-DSTELLA_GC_DEBUG_MODE=ON|OFF` Enable logging for GC
* `-DSTELLA_GC_MOVE_ALWAYS=ON|OFF` This options tells GC to initiate the marking-and-moving phase every time `gc_alloc` is called
* `-DBUILD_WITH_SANITIZERS=ON|OFF` Build everything with address sanitizers
