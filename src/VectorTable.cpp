// Minimal Cortex-M0+ vector table for SAMD21E18A
// Placed at start of flash (0x00000000) via .vectors section

#include <cstdint>
#include "UsbCDC.h"
#include "libs/cmsis/samd21a/include/samd21e18a.h"
#include "libs/cmsis/Core/Include/cmsis_gcc.h"
#include "time.h"


extern "C" {
// Reset handler — defined in startup.cpp
void Reset_Handler(void);

// Default stub handler for unimplemented interrupts
void Default_Handler(void) {
    for (;;) {}
}

// HardFault diagnostic handler — reads fault status registers and leaves state visible to GDB
void HardFault_Handler(void) {
    volatile uint32_t hfsr = *(volatile uint32_t*)(SCS_BASE + 0x024C);
    volatile uint32_t bfar = *(volatile uint32_t*)(SCS_BASE + 0x0238);

    __asm__ volatile(
        "mov r0, %0\n"
        "mov r1, %1\n"
        "str r0, [sp]\n"
        "str r1, [sp, #4]\n"
        : : "r"(hfsr), "r"(bfar) : "r0","r1","memory"
    );

    for (;;) {
        __asm__ volatile("bkpt #0");
    }
}

// USB interrupt handler — calls Arduino-style ISR handler.
// All USB processing (EP0 setup, CDC data TX/RX, descriptors) happens here.
// No polling or task() calls needed from main loop.

void USB_Handler(void) {
    USBSerial_ISRHandler();
}

// Forward declarations with weak aliases to Default_Handler
__attribute__((weak, alias("Default_Handler"))) void PM_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void SYSCTRL_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void WDT_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void RTC_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void EIC_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void NVMCTRL_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void DMAC_Handler(void);

// SysTick — fires every 1 ms (see Chip::usb_init()). Bumps the millis counter.
void SysTick_Handler(void) {
    s_millis_counter++;
}

__attribute__((weak, alias("Default_Handler"))) void EVSYS_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void SERCOM0_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void SERCOM1_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void SERCOM2_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void SERCOM3_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void RESERVED_14_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void RESERVED_15_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void TCC0_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void TCC1_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void TCC2_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void TC3_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void TC4_Handler(void);
__attribute__((weak, alias("Default_Handler"))) void TC5_Handler(void);

extern "C" uint32_t _estack;

// Cortex-M vector table — must be first in .vectors section at 0x00000000
using VectorEntry = void(*)();

__attribute__((section(".vectors"), used))
const VectorEntry g_vectorTable[] = {
    // Initial stack pointer value (RAM end)
    reinterpret_cast<VectorEntry>(&_estack),

    // Core exception handlers (indices 1-15)
    &Reset_Handler,                            // 1: Reset
    &Default_Handler,                          // 2: NMI
    &HardFault_Handler,                        // 3: HardFault
    &Default_Handler,                          // 4: MemManage (Reserved on M0+)
    &Default_Handler,                          // 5: BusFault (Reserved on M0+)
    &Default_Handler,                          // 6: UsageFault (Reserved on M0+)
    &Default_Handler,                          // 7: Reserved
    &Default_Handler,                          // 8: Reserved
    &Default_Handler,                          // 9: Reserved
    &Default_Handler,                          // 10: Reserved
    &Default_Handler,                          // 11: SVCall
    &Default_Handler,                          // 12: DebugMonitor
    &Default_Handler,                          // 13: Reserved
    &Default_Handler,                          // 14: PendSV
    &SysTick_Handler,                          // 15: SysTick

    // Peripheral interrupts — starting at index 16 = core count (IRQn 0+)
    &PM_Handler,            // IRQn 0: Power Manager
    &SYSCTRL_Handler,       // IRQn 1: System Control
    &WDT_Handler,           // IRQn 2: Watchdog Timer
    &RTC_Handler,           // IRQn 3: Real-Time Counter
    &EIC_Handler,           // IRQn 4: External Interrupt Controller
    &NVMCTRL_Handler,       // IRQn 5: Non-Volatile Memory Controller
    &DMAC_Handler,          // IRQn 6: Direct Memory Access Controller
    &USB_Handler,           // IRQn 7: Universal Serial Bus (CDC-ACM)
    &EVSYS_Handler,         // IRQn 8: Event System Interface
    &SERCOM0_Handler,       // IRQn 9: Serial Communication Interface 0
    &SERCOM1_Handler,       // IRQn 10: Serial Communication Interface 1
    &SERCOM2_Handler,       // IRQn 11: Serial Communication Interface 2
    &SERCOM3_Handler,       // IRQn 12: Serial Communication Interface 3
    &RESERVED_14_Handler,   // IRQn 13-14: Reserved (no SERCOM4/5 on E18A)
    &RESERVED_15_Handler,
    &TCC0_Handler,          // IRQn 15: Timer Counter Control 0
    &TCC1_Handler,          // IRQn 16: Timer Counter Control 1
    &TCC2_Handler,          // IRQn 17: Timer Counter Control 2
    &TC3_Handler,           // IRQn 18: Basic Timer Counter 3
    &TC4_Handler,           // IRQn 19: Basic Timer Counter 4
    &TC5_Handler,           // IRQn 20: Basic Timer Counter 5

    // Remaining entries (IRQn 21-47) — all default handlers
    &Default_Handler,   // 21: Reserved
    &Default_Handler,   // 22: Reserved
    &Default_Handler,   // 23: ADC
    &Default_Handler,   // 24: AC
    &Default_Handler,   // 25: DAC
    &Default_Handler,   // 26: PTC
    &Default_Handler,   // 27: I2S
    &Default_Handler,   // 28-47: more reserved/peripheral
    &Default_Handler,
    &Default_Handler,
    &Default_Handler,
};

} // extern "C"
