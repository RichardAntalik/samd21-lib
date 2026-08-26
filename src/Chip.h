#pragma once
#include <cstdint>
#include <cstdarg>
#include <type_traits>

// Official CMSIS headers (provides IRQn_Type, component headers, base addresses)
#include "libs/cmsis/Core/Include/cmsis_gcc.h"
#include "libs/cmsis/samd21a/include/samd21e18a.h"


extern "C" { void NVIC_EnableIRQ(IRQn_Type irq); }

// ============================================================================
// Compatibility macros - map code's full register names to old Atmel short names
// Code uses: GCLK_REGS->GCLK_CLKCTRL, SYSCTRL_REGS->SYSCTRL_OSC8M, etc.
// Old CMSIS uses: GCLK->CLKCTRL, SYSCTRL->OSC8M, etc. (no peripheral prefix)
// ============================================================================

#ifndef GCLK_CLKCTRL
#define GCLK_CLKCTRL  CLKCTRL
#endif
#ifndef GCLK_STATUS
#define GCLK_STATUS   STATUS
#endif
#ifndef GCLK_GENCTRL
#define GCLK_GENCTRL  GENCTRL
#endif
#ifndef SYSCTRL_OSC8M
#define SYSCTRL_OSC8M  OSC8M
#endif
#ifndef SYSCTRL_PCLKSR
#define SYSCTRL_PCLKSR  PCLKSR
#endif

#include "Types.h"
#include "time.h"
#include "Scheduler.h"
#include "Pin.h"
#include "ResourceManager.h"
#include "I2cMaster.h"
#include "UsbPrint.h"

#include <cstdint>
#include <cstddef>
#include <cstring>

// ============================================================================
// Chip - Root class; singleton/instance mapping pins to the chip
//
// Each pin is a unique template instantiation, enabling compile-time capability
// checks. Users access pins directly: chip.PA10.use_out()
// ============================================================================

class Chip {
public:
    static Chip& instance() {
        static Chip c;
        return c;
    }

    // ====================================================================
    // PA pins (Port A) - each is a unique Pin<PORTA_BASE_ADDR, N> type
    // ====================================================================

    Pin<PORTA_BASE_ADDR, 0> PA00;
    Pin<PORTA_BASE_ADDR, 1> PA01;
    Pin<PORTA_BASE_ADDR, 2> PA02;
    Pin<PORTA_BASE_ADDR, 3> PA03;
    Pin<PORTA_BASE_ADDR, 4> PA04;
    Pin<PORTA_BASE_ADDR, 5> PA05;
    Pin<PORTA_BASE_ADDR, 6> PA06;
    Pin<PORTA_BASE_ADDR, 7> PA07;
    Pin<PORTA_BASE_ADDR, 8> PA08;
    Pin<PORTA_BASE_ADDR, 9> PA09;
    Pin<PORTA_BASE_ADDR, 10> PA10;
    Pin<PORTA_BASE_ADDR, 11> PA11;
    Pin<PORTA_BASE_ADDR, 12> PA12;
    Pin<PORTA_BASE_ADDR, 13> PA13;
    Pin<PORTA_BASE_ADDR, 14> PA14;
    Pin<PORTA_BASE_ADDR, 15> PA15;
    Pin<PORTA_BASE_ADDR, 16> PA16;
    Pin<PORTA_BASE_ADDR, 17> PA17;
    Pin<PORTA_BASE_ADDR, 18> PA18;
    Pin<PORTA_BASE_ADDR, 19> PA19;
    Pin<PORTA_BASE_ADDR, 20> PA20;
    Pin<PORTA_BASE_ADDR, 21> PA21;
    Pin<PORTA_BASE_ADDR, 22> PA22;
    Pin<PORTA_BASE_ADDR, 23> PA23;
    Pin<PORTA_BASE_ADDR, 24> PA24;
    Pin<PORTA_BASE_ADDR, 25> PA25;
    Pin<PORTA_BASE_ADDR, 27> PA27;
    Pin<PORTA_BASE_ADDR, 28> PA28;
    Pin<PORTA_BASE_ADDR, 30> PA30;
    Pin<PORTA_BASE_ADDR, 31> PA31;

    // ====================================================================
    // PB pins (Port B) - each is a unique Pin<PORTB_BASE_ADDR, N> type
    // ====================================================================

    Pin<PORTB_BASE_ADDR, 0> PB00;
    Pin<PORTB_BASE_ADDR, 1> PB01;
    Pin<PORTB_BASE_ADDR, 2> PB02;
    Pin<PORTB_BASE_ADDR, 3> PB03;
    Pin<PORTB_BASE_ADDR, 4> PB04;
    Pin<PORTB_BASE_ADDR, 5> PB05;
    Pin<PORTB_BASE_ADDR, 6> PB06;
    Pin<PORTB_BASE_ADDR, 7> PB07;
    Pin<PORTB_BASE_ADDR, 8> PB08;
    Pin<PORTB_BASE_ADDR, 9> PB09;
    Pin<PORTB_BASE_ADDR, 10> PB10;
    Pin<PORTB_BASE_ADDR, 11> PB11;
    Pin<PORTB_BASE_ADDR, 12> PB12;
    Pin<PORTB_BASE_ADDR, 13> PB13;
    Pin<PORTB_BASE_ADDR, 14> PB14;
    Pin<PORTB_BASE_ADDR, 15> PB15;
    Pin<PORTB_BASE_ADDR, 16> PB16;
    Pin<PORTB_BASE_ADDR, 17> PB17;
    Pin<PORTB_BASE_ADDR, 22> PB22;
    Pin<PORTB_BASE_ADDR, 23> PB23;
    Pin<PORTB_BASE_ADDR, 30> PB30;
    Pin<PORTB_BASE_ADDR, 31> PB31;

    // ====================================================================
    // Power management
    // ====================================================================

    Status power_set(SleepMode mode) {
        uint32_t idle_select = 0;
        switch (mode) {
        case SleepMode::IDLE_0:
            idle_select = SCB_SCR_SLEEPDEEP_Msk;
            break;
        case SleepMode::IDLE_1:
            idle_select = 0x01;
            break;
        case SleepMode::IDLE_2:
            idle_select = 0x02;
            break;
        case SleepMode::STANDBY:
            idle_select = SCB_SCR_SLEEPDEEP_Msk;
            break;
        }

        SCB->SCR = (SCB->SCR & ~SCB_SCR_SLEEPDEEP_Msk) | idle_select;
        __asm__ volatile("wfi");

        return Status::OK;
    }

    // ====================================================================
    // Resource manager access
    // ====================================================================

    ResourceManager& resources() {
        return ResourceManager::instance();
    }

    // ====================================================================
    // System initialization
    // ====================================================================

    // ====================================================================
    // USB serial init - fully interrupt-driven, no polling needed.
    // After this: chip.usb.print("hello") sends data over USB.
    // All USB processing happens inside the ISR. No tusb_task() calls.
    // ====================================================================

    void usb_init() {
        chip_usb_cdc.usb_init();

        // SysTick: COUNTFLAG used by delay(). Handler is stubbed.
        SysTick->LOAD = (48000000UL / 1000UL) - 1UL;
        SysTick->VAL  = 0UL;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk   // CPU clock (48MHz)
                        | SysTick_CTRL_TICKINT_Msk   // Enable interrupt (handler is stubbed)
                        | SysTick_CTRL_ENABLE_Msk;   // Start timer
    }

    void init() {
        Sysctrl* sysctrl = SYSCTRL;
        Gclk* gclk       = GCLK;
        Nvmctrl* nvmctrl = NVMCTRL;

        // Enable OSC8M with factory calibration
        uint8_t osc8m_cal = *(volatile uint8_t*)(NVMCTRL_USER + 0x22) >> 1;
        sysctrl->OSC8M.reg = (sysctrl->OSC8M.reg & ~(SYSCTRL_OSC8M_PRESC_Msk | SYSCTRL_OSC8M_CALIB_Msk))
                            | SYSCTRL_OSC8M_ENABLE
                            | (osc8m_cal << 8);
        while (!(sysctrl->PCLKSR.reg & SYSCTRL_PCLKSR_OSC8MRDY)) {}

        // Flash Read Wait State
        nvmctrl->CTRLB.reg |= (1 << 1);

        // ERRATA 9905: Clear ONDEMAND bit before configuring DFLL
        sysctrl->DFLLCTRL.reg &= ~SYSCTRL_DFLLCTRL_ONDEMAND;
        while (!(sysctrl->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)) {}

        // Write Coarse + Fine calibration
        // INTENTIONALLY HARDCODED!!! There are bad chips out there!
        sysctrl->DFLLVAL.reg = SYSCTRL_DFLLVAL_FINE(512) | SYSCTRL_DFLLVAL_COARSE(0x1D);
        while (!(sysctrl->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)) {}

        // Set the multiplier for USB SOF recovery
        sysctrl->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(31) | 
                               SYSCTRL_DFLLMUL_FSTEP(10) | 
                               SYSCTRL_DFLLMUL_MUL(48000);

        // Enable Closed Loop mode along with USBCRM
        sysctrl->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_BPLCKC |
                        SYSCTRL_DFLLCTRL_CCDIS  | 
                        //SYSCTRL_DFLLCTRL_USBCRM  
                        //SYSCTRL_DFLLCTRL_MODE    
                        SYSCTRL_DFLLCTRL_ENABLE;
        while (!(sysctrl->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)) {}

        // GCLK0 -> DFLL48M
        gclk->GENDIV.reg = GCLK_GENDIV_ID(0) | GCLK_GENDIV_DIV(1);
        while (gclk->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

        gclk->GENCTRL.reg = GCLK_GENCTRL_ID(0) | 
                            GCLK_GENCTRL_SRC_DFLL48M | 
                            GCLK_GENCTRL_IDC | 
                            GCLK_GENCTRL_GENEN;
        while (gclk->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

        // GCLK1 -> DFLL48M
        gclk->GENDIV.reg = GCLK_GENDIV_ID(1) | GCLK_GENDIV_DIV(1);
        while (gclk->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

        gclk->GENCTRL.reg = GCLK_GENCTRL_ID(1) | 
                            GCLK_GENCTRL_SRC_DFLL48M | 
                            GCLK_GENCTRL_IDC | 
                            GCLK_GENCTRL_GENEN;
        while (gclk->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

        usb_init();
    }

    // Legacy stub - no longer needed with interrupt-driven stack
    void usb_task() {}

    // ====================================================================
    // I2C factory - creates an I2C master instance for the given SERCOM
    //
    // Pins must already be configured via pin.use_sercom<Peripheral::IIC>().
    // The SERCOM resource is reserved on first call. Returns reference to
    // a persistent instance (caller should cache it).
    // ====================================================================

    template <SercomInst S>
    I2cMaster<S>& i2c() {
        static_assert(static_cast<uint8_t>(S) <= 5, "I2C: SERCOM out of range for this chip variant");
        ResourceManager& rm = ResourceManager::instance();
        if (rm.is_sercom_available(S) != Status::OK) {
            // Already reserved — still return the instance so caller can use it
        }
        rm.reserve_sercom(S);
        static I2cMaster<S> inst;
        return inst;
    }

    // Access the USB serial instance - provides print/println/read/available methods
    USBPrint usb;

    // Cooperative periodic callback scheduler - see Scheduler.h.
    // Register with chip.scheduler.every(ms, fn[, user]); advance with
    // chip.scheduler.tick() once per loop().
    Scheduler scheduler;

    Chip() = default;
    ~Chip() = default;

    Chip(const Chip&) = delete;
    Chip& operator=(const Chip&) = delete;
};

// ============================================================================
// Global chip instance - use as: chip.PA10.use_out(), chip.usb.print("hello")
// ============================================================================

inline Chip& chip = Chip::instance();

// ============================================================================
// Global serial instance - Arduino-compatible `Serial` (a Stream over USB CDC).
// Lets Arduino-style libraries (e.g. SerialConsole) work unmodified.
// ============================================================================

inline Stream& Serial = chip.usb;

// ============================================================================
// Global printf() / eprintf() - C-style formatted output routed to Serial
// (USB CDC). Simplified specifier set, see Stream::vprintf().
// ============================================================================

inline int printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    size_t n = Serial.vprintf(fmt, ap);
    va_end(ap);
    return static_cast<int>(n);
}

inline int eprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    size_t n = Serial.vprintf(fmt, ap);
    va_end(ap);
    return static_cast<int>(n);
}

