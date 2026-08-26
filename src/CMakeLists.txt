# Chip library - startup, vector table, runtime support
# USB CDC is now header-only (UsbCDC.h), no separate compilation needed
set(CHIP_SOURCES
    startup.cpp
    VectorTable.cpp
    RuntimeSupport.cpp
)

add_library(chip_lib ${CHIP_SOURCES})
target_include_directories(chip_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/cmsis/Core/Include
)
