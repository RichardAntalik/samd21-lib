cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED SAMD21_LIB)
    if(DEFINED ENV{SAMD21_LIB})
        set(SAMD21_LIB "$ENV{SAMD21_LIB}")
    else()
        # Stamped by samd-new at generation time; override with
        # -DSAMD21_LIB=... or the SAMD21_LIB env var.
        set(SAMD21_LIB "__LIB__")
    endif()
endif()

project(__NAME__ LANGUAGES CXX C ASM)

set(APP_SOURCES main.cpp)

# Extra project-local libraries - managed by tools/new/samd-add-lib
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libs.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/libs.cmake)
endif()

include(${SAMD21_LIB}/tools/build/samd-app.cmake)
