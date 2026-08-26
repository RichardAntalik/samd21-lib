# ============================================================================
# samd-app.cmake - shared build logic for SAMD21 app projects.
#
# The caller must have:
#   - called project(<name> LANGUAGES CXX C ASM)
#   - set(SAMD21_LIB <absolute path to this repo>)
#   - set(APP_SOURCES <app source files>)
#
# Extra project-local libraries: if the project dir contains libs.cmake the
# caller should include() it before this file (it is managed by
# tools/new/samd-add-lib).
# ============================================================================

# Export the clangd/IDE compilation database (build/compile_commands.json)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build profile: fast (default) = speed, debug = symbols
set(PROFILE "fast" CACHE STRING "Build profile: fast, debug")

# Toolchain
set(TARGET_CPU "-mcpu=cortex-m0plus -mthumb")
set(CXX_BASE_FLAGS "${TARGET_CPU} -std=c++17 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -D__SAMD21E18A__")
set(C_BASE_FLAGS "${TARGET_CPU} -std=c99 -ffunction-sections -fdata-sections -D__SAMD21E18A__")

if(PROFILE STREQUAL "debug")
  set(PROFILE_FLAGS "-O0 -g3 -gdwarf-4")
else()
  set(PROFILE_FLAGS "-O2")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_BASE_FLAGS} ${PROFILE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_BASE_FLAGS} ${PROFILE_FLAGS}")

# Linker flags
set(LINKER_FLAGS "-T ${SAMD21_LIB}/tools/build/link.ld -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS}")

# Include directories for the whole project. Headers are included flat:
# #include "Chip.h", #include "SerialConsole.h" - never "src/Chip.h".
include_directories(
    ${SAMD21_LIB}/src
    ${SAMD21_LIB}/src/libs/cmsis/Core/Include
    ${SAMD21_LIB}/src/libs/cmsis/samd21a/include
)

# Libs root: every directory under it is added to the include path, so lib
# headers are includable flat with no per-project wiring. Sibling layout by
# default (<root>/samd21-lib <root>/Foo), override with $SAMD21_LIBS.
#
# Plain libs - directories WITHOUT a CMakeLists.txt (header-only, or .h +
# loose .c/.cpp) - work completely out of the box: include path above,
# and their top-level *.c/*.cpp are compiled straight into firmware.elf.
#
# Directories WITH a CMakeLists.txt are never touched here (they are the
# central repo, generated app projects, or managed libs) - a managed lib
# that defines its own targets is still wired per-project with
# samd-add-lib.
if(NOT DEFINED SAMD21_LIBS)
    if(DEFINED ENV{SAMD21_LIBS})
        set(SAMD21_LIBS "$ENV{SAMD21_LIBS}")
    else()
        get_filename_component(SAMD21_LIBS "${SAMD21_LIB}/.." ABSOLUTE)
    endif()
endif()
get_filename_component(SAMD21_LIB_REAL ${SAMD21_LIB} REALPATH)
include_directories(${SAMD21_LIBS})
set(_SAMD_AUTO_LIB_SOURCES "")
file(GLOB _SAMD_LIB_DIRS "${SAMD21_LIBS}/*")
foreach(_lib_dir ${_SAMD_LIB_DIRS})
    if(IS_DIRECTORY ${_lib_dir})
        get_filename_component(_lib_dir_real ${_lib_dir} REALPATH)
        if(NOT _lib_dir_real STREQUAL SAMD21_LIB_REAL)
            include_directories(${_lib_dir})
            if(NOT EXISTS ${_lib_dir}/CMakeLists.txt)
                file(GLOB _lib_sources "${_lib_dir}/*.c" "${_lib_dir}/*.cpp")
                list(APPEND _SAMD_AUTO_LIB_SOURCES ${_lib_sources})
            endif()
        endif()
    endif()
endforeach()

# Chip library, built from the central repo into this project's build tree
add_subdirectory(${SAMD21_LIB}/src ${CMAKE_BINARY_DIR}/src)

# Application firmware (skipped by pure-library builds, e.g. this repo
# without a main.cpp)
if(DEFINED APP_SOURCES)
    add_executable(firmware.elf ${APP_SOURCES} ${_SAMD_AUTO_LIB_SOURCES})
    target_link_libraries(firmware.elf PRIVATE -Wl,--whole-archive chip_lib -Wl,--no-whole-archive)

    # Generate binary from ELF
    add_custom_command(TARGET firmware.elf POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:firmware.elf> ${CMAKE_BINARY_DIR}/firmware.bin
        COMMENT "Creating firmware.bin from firmware.elf"
    )

    # Show size information after linking
    add_custom_command(TARGET firmware.elf POST_BUILD
        COMMAND arm-none-eabi-size $<TARGET_FILE:firmware.elf>
        COMMENT "Size info for firmware.elf"
    )
endif()
