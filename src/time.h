#pragma once
#include "libs/cmsis/Core/Include/cmsis_gcc.h"
#include <cstdint>

// ============================================================================
// Millisecond counter - bumped by the 1 kHz SysTick interrupt.
// SysTick is configured with a 1 ms period in Chip::usb_init(); the handler
// (VectorTable.cpp) increments this counter on every tick.
// ============================================================================
inline volatile uint32_t s_millis_counter = 0;

inline unsigned long millis() {
    return s_millis_counter;
}

// ============================================================================
// Standalone delay function - blocks for the specified number of milliseconds.
// Uses SysTick COUNTFLAG which is already configured with 1ms period in
// Chip::usb_init(). USB event processing runs via SysTick interrupt, so this
// delay works correctly even though it's a busy-wait (interrupts still fire).
// ============================================================================

inline void delay(uint32_t ms) {
    volatile uint32_t flags = SysTick->CTRL;
    while (ms--) {
        do {
            // COUNTFLAG is cleared by reading CTRL, then set on next overflow
        } while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
    }
}
