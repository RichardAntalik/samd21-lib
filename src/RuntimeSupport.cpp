// Minimal bare-metal runtime support for SAMD21DA1 USB CDC Serial
// Provides missing symbols not linked by default with -nostdlib

#include <cstdint>
#include <cstddef>
#include "libs/cmsis/samd21a/include/samd21e18a.h"

extern "C" {

// --- NVIC Interrupt Enable/Disable ---
// CMSIS provides inline versions via __NVIC_EnableIRQ/__NVIC_DisableIRQ.
// We undefine the macros and provide symbol-level wrappers for code 
// that calls them without including CMSIS headers (e.g., TinyUSB OSAL).

#undef NVIC_EnableIRQ
__attribute__((weak)) void NVIC_EnableIRQ(IRQn_Type irq) {
    uint32_t idx = static_cast<uint32_t>(irq) / 32;
    uint32_t bit = static_cast<uint32_t>(irq) % 32;
    NVIC->ISER[idx] = (1U << bit);
}

#undef NVIC_DisableIRQ
__attribute__((weak)) void NVIC_DisableIRQ(IRQn_Type irq) {
    uint32_t idx = static_cast<uint32_t>(irq) / 32;
    uint32_t bit = static_cast<uint32_t>(irq) % 32;
    NVIC->ICER[idx] = (1U << bit);
}

} // extern "C"
