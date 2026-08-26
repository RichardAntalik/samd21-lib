
# SerialConsole - added by samd-add-lib (2026-08-26)
if(NOT DEFINED SERIALCONSOLE_LIB)
    if(DEFINED ENV{SAMD21_LIBS})
        set(SERIALCONSOLE_LIB "$ENV{SAMD21_LIBS}/SerialConsole")
    else()
        set(SERIALCONSOLE_LIB "/home/me/tools/SerialConsole")
    endif()
endif()
# header-only: include both the libs root and the lib itself
include_directories(${SERIALCONSOLE_LIB}/.. ${SERIALCONSOLE_LIB})
