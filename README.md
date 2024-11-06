# GC for Stella

## Build instructions

### Simplest example

1. Configure CMake: `cmake -B build`
2. Build with CMake: `cmake --build build`

This will generate `build/libstella_gc.a` for the GC (without runtime) and `build/libstella_runtime.a` for the Stella runtime library.

You can use the GC object file independently -- just link it with your own runtime and have fun!

### Build with Stella programs

`-DBUILD_STELLA_PROGRAMS=ON -DSTELLA_COMPILER=/path/to/stella/compiler`

If you add these two arguments to the "Configure CMake" command, CMake will also build Stella programs from the `examples/` directory. It will statically link them with the runtime and GC. Built binaries will be located under `build/stella_examples`.

For each Stella program, two binaries will be generated. For instance, for the program `exp.st` files `exp` and `exp__epsilon_gc` will be generated. The former is linked with the actual GC implementation, the latter -- with epsilon GC that does nothing.

### Additional development options

GC parameters:

* `-DMAX_ALLOC_SIZE=1024` Defines the size of available memory (in bytes)

Stella options:

* `-DSTELLA_DEBUG=ON|OFF` Defines the STELLA_DEBUG macro
* `-DSTELLA_GC_STATS=ON|OFF` Defines the STELLA_GC_STATS macro
* `-DSTELLA_RUNTIME_STATS=ON|OFF` Defines the STELLA_RUNTIME_STATS macro

Statis checks for GC:

* `-DSTRICT_BUILD_MODE=ON|OFF` Build the GC source files with `-Wall -Wpedantic ...`

Runtime checks for GC:

* `-DSTELLA_GC_DEBUG_MODE=ON|OFF` Enable logging for GC
* `-DSTELLA_GC_MOVE_ALWAYS=ON|OFF` This options tells GC to marking-and-moving phase every time an object is allocated is called
* `-DBUILD_WITH_SANITIZERS=ON|OFF` Build everything with address sanitizers

## GC Statistics Example

```
$ echo 5 | ./build/stella_examples/bin/factorial_functional

...

------------------------------------------------------------
Garbage collector (GC) statistics:
MAX_ALLOC_SIZE:                  10,000 bytes
Total memory allocation:         14,368 bytes (888 objects)
Maximum residency:               4,056 bytes
Total number of GC cycles:       888 times
Maximum number of roots:         41
```
