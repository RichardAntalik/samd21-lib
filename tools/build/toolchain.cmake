set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Cross-compiler paths
set(TOOLCHAIN_PREFIX arm-none-eabi-)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++ CACHE STRING "C++ compiler")
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy CACHE STRING "Objcopy tool")
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size CACHE STRING "Size tool")

# Disable compiler tests (bare-metal has no libc for linking test programs)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Never search for system libraries or use sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
