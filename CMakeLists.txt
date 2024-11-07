cmake_minimum_required(VERSION 3.10)
project(StellaGC C)

# ------------------------------------------------------------
# --- Options

# GC parameters:
set(MAX_ALLOC_SIZE "1024" CACHE STRING "MAX_ALLOC_SIZE in bytes")

# Stella options:
option(STELLA_DEBUG "Define STELLA_DEBUG" OFF)
option(STELLA_GC_STATS "Define STELLA_GC_STATS" OFF)
option(STELLA_RUNTIME_STATS "Define STELLA_RUNTIME_STATS" OFF)

# Static checks for GC
option(STRICT_BUILD_MODE "Enable strict compiler checks for GC" OFF)

# Runtime checks for GC
option(BUILD_WITH_SANITIZERS "Build with sanitizers" OFF)
option(STELLA_GC_DEBUG_MODE "Enable GC debuging mode" OFF)
option(STELLA_GC_MOVE_ALWAYS "Perform a moving phase on every allocation" OFF)

# Tests
option(TEST_ON_STELLA_PROGRAMS "Build and run stella programs from the examples directory" OFF)
set(STELLA_COMPILER "" CACHE STRING "Path to the PET compiler")

# ------------------------------------------------------------
# --- Common Definitions

set(WERROR_OPTIONS "-Wall" "-Wextra" "-Wpedantic" "-Werror")
set(SANITIZER_OPTIONS "-fsanitize=address" "-fno-omit-frame-pointer")

file(GLOB GC_SOURCES src/*.c)
file(GLOB STELLA_RUNTIME_SOURCES runtime/runtime.c)
file(GLOB STELLA_EPSILON_GC_SOURCES runtime/epsilon_gc.c)
file(GLOB STELLA_PROGRAMS examples/*.st)

# ------------------------------------------------------------
# --- Main targets for GC and runtime

include_directories(include include/stella)

add_library(stella_gc STATIC ${GC_SOURCES})
add_library(stella_epsilon_gc STATIC ${STELLA_EPSILON_GC_SOURCES})
add_library(stella_runtime STATIC ${STELLA_RUNTIME_SOURCES})

set_target_properties(
    stella_gc stella_runtime stella_epsilon_gc
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_compile_options(stella_runtime PRIVATE -Wno-everything)
target_compile_options(stella_epsilon_gc PRIVATE -Wno-everything)

# ------------------------------------------------------------
# --- Add definitions based on given options

if(TEST_ON_STELLA_PROGRAMS AND NOT STELLA_COMPILER)
    message(FATAL_ERROR "STELLA_COMPILER must be defined if TEST_ON_STELLA_PROGRAMS is ON.")
endif()

if(MAX_ALLOC_SIZE)
    add_compile_definitions(MAX_ALLOC_SIZE=${MAX_ALLOC_SIZE})
endif(MAX_ALLOC_SIZE)

if(BUILD_WITH_SANITIZERS)
    target_compile_options(stella_gc PRIVATE ${SANITIZER_OPTIONS})
    target_compile_options(stella_runtime PRIVATE ${SANITIZER_OPTIONS})
    target_compile_options(stella_epsilon_gc PRIVATE ${SANITIZER_OPTIONS})
    string(APPEND CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
endif(BUILD_WITH_SANITIZERS)

if(STRICT_BUILD_MODE)
    target_compile_options(stella_gc PRIVATE ${WERROR_OPTIONS})
endif()

if(STELLA_GC_DEBUG_MODE)
    add_compile_definitions(STELLA_GC_DEBUG_MODE)
endif(STELLA_GC_DEBUG_MODE)

if(STELLA_GC_MOVE_ALWAYS)
    add_compile_definitions(STELLA_GC_MOVE_ALWAYS)
endif(STELLA_GC_MOVE_ALWAYS)

if(STELLA_DEBUG)
    add_compile_definitions(STELLA_DEBUG)
endif(STELLA_DEBUG)

if(STELLA_GC_STATS)
    add_compile_definitions(STELLA_GC_STATS)
endif(STELLA_GC_STATS)

if(STELLA_RUNTIME_STATS)
    add_compile_definitions(STELLA_RUNTIME_STATS)
endif(STELLA_RUNTIME_STATS)


# ------------------------------------------------------------
# --- Tests

# --------------------
# --- Targets for compiling user programs

function(add_user_program_executable target_name gc_lib program_c_file)
    add_executable(${target_name} ${program_c_file})
    target_link_libraries(${target_name} ${gc_lib} stella_runtime)
    set_target_properties(${TARGET_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/stella_examples/bin
    )
    if(BUILD_WITH_SANITIZERS)
        target_compile_options(${TARGET_NAME} PRIVATE ${SANITIZER_OPTIONS})
    endif()
endfunction()

if(TEST_ON_STELLA_PROGRAMS)
    foreach(STELLA_PROGRAM ${STELLA_PROGRAMS})
        get_filename_component(PROGRAM_NAME ${STELLA_PROGRAM} NAME_WE)
        set(STELLA_PROG_C_OUT ${CMAKE_BINARY_DIR}/stella_examples/${PROGRAM_NAME}.c)
        set(TARGET_NAME ${PROGRAM_NAME})
        set(EPSILON_GC_TARGET_NAME ${TARGET_NAME}__epsilon)

        # Define target that compiles the given stella program into C file
        add_custom_command(
            OUTPUT ${STELLA_PROG_C_OUT}
            COMMAND ${STELLA_COMPILER} compile < ${STELLA_PROGRAM} > ${STELLA_PROG_C_OUT}
            DEPENDS ${STELLA_PROGRAM}
            COMMENT "Compiling ${STELLA_PROGRAM} with Stella compiler"
        )
        # Add executable target and link with libraries
        add_user_program_executable(${TARGET_NAME} stella_gc ${STELLA_PROG_C_OUT})
        # Add executable for epsilon gc
        add_user_program_executable(${EPSILON_GC_TARGET_NAME} stella_epsilon_gc ${STELLA_PROG_C_OUT})
    endforeach()
endif()
