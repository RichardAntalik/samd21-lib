/**
 * \file
 * \brief Minimal system_samd21.h for old Atmel CMSIS compatibility
 * 
 * This header satisfies the #include in samd21e18a.h when using chip/ library
 * which provides its own startup code. No SystemInit/SystemCoreClock needed.
 */

#pragma once

/* System frequency definitions - set by application if needed */
#ifndef SYSTEM_CLOCK_HZ
#define SYSTEM_CLOCK_HZ   48000000UL
#endif

#ifndef CPU_CORE_CLOCK_HZ
#define CPU_CORE_CLOCK_HZ  48000000UL
#endif
