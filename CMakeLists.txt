cmake_minimum_required(VERSION 3.10)
project(StellaGC C)

include_directories(include include/stella)

file(GLOB GC_SOURCES src/*.c)

file(GLOB STELLA_RUNTIME_SOURCES runtime/*.c)

add_library(stella_gc STATIC ${GC_SOURCES})
add_library(stella_runtime STATIC ${STELLA_RUNTIME_SOURCES})

set_target_properties(
    stella_gc stella_runtime
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_compile_options(stella_runtime PRIVATE -Wno-everything)

# Strict Mode settings
option(STRICT_MODE "Enable strict compiler checks for GC" OFF)

set(SANITIZER_OPTIONS "-fsanitize=address" "-fno-omit-frame-pointer")
set(WERROR_OPTIONS "-Wall" "-Wextra" "-Wpedantic" "-Werror")

if(STRICT_MODE)
    target_compile_options(stella_gc PRIVATE ${WERROR_OPTIONS} ${SANITIZER_OPTIONS})
    target_compile_options(stella_runtime PRIVATE ${SANITIZER_OPTIONS} "-DSTELLA_DEBUG")
    string(APPEND CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
endif()

# ------------------------------------------------------------
# --- Targets for compiling user programs

file(GLOB STELLA_PROGRAMS examples/*.st)

option(BUILD_STELLA_PROGRAMS "Build stella programs from the examples directory" OFF)
set(STELLA_COMPILER "" CACHE STRING "Path to the PET compiler")

if(BUILD_STELLA_PROGRAMS AND NOT STELLA_COMPILER)
    message(FATAL_ERROR "STELLA_COMPILER must be defined if BUILD_STELLA_PROGRAMS is ON.")
endif()

if(BUILD_STELLA_PROGRAMS)
    foreach(STELLA_PROGRAM ${STELLA_PROGRAMS})
        get_filename_component(PROGRAM_NAME ${STELLA_PROGRAM} NAME_WE)
        set(STELLA_PROG_C_OUT ${CMAKE_BINARY_DIR}/stella_examples/${PROGRAM_NAME}.c)
        set(TARGET_NAME examples__${PROGRAM_NAME})

        # Define target that compiles the given stella program into C file
        add_custom_command(
            OUTPUT ${STELLA_PROG_C_OUT}
            COMMAND ${STELLA_COMPILER} compile < ${STELLA_PROGRAM} > ${STELLA_PROG_C_OUT}
            DEPENDS ${STELLA_PROGRAM}
            COMMENT "Compiling ${STELLA_PROGRAM} with Stella compiler"
        )

        # Add executable target and link with libraries
        add_executable(${TARGET_NAME} ${STELLA_PROG_C_OUT})
        target_link_libraries(${TARGET_NAME} stella_gc stella_runtime)
        set_target_properties(${TARGET_NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/stella_examples/bin
        )

        if(STRICT_MODE)
            target_compile_options(${TARGET_NAME} PRIVATE ${SANITIZER_OPTIONS})
        endif()
    endforeach()
endif()
