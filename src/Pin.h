#pragma once
#include <cstdint>
#include <type_traits>

#include "Types.h"
#include "ResourceManager.h"
#include "PinMap.h"

// ============================================================================
// Per-pin register page size on SAMD21 PORT (PINCFG byte + reserved space)
#define SAMD21_PORT_PIN_PAGE_SIZE PORT_PINCFG_OFFSET

// IOBUS offset to PMUX register within each pin's page
#define SAMD21_PORT_PMUX_OFFSET     PORT_PMUX_OFFSET

// Pin template class - Each pin is a unique type known at compile time
// Template parameters: PortAddr (integer base address), PinNum (0-31)
// ============================================================================

template <uint32_t PortAddr, uint8_t PinNum>
class Pin {
public:
    // Pin index into PIN_MAP (0-31 for PORTA, 32-63 for PORTB)
    static constexpr uint8_t pin_map_index() {
        return PortAddr == PORT_IOBUS_BASE_ADDR ? PinNum : 32 + PinNum;
    }

    // =====================================================================
    // Compile-time capability lookups (constexpr - resolved at compile time)
    // =====================================================================

    static constexpr AdcChan adc_channel() {
        return PIN_MAP[pin_map_index()].adc_chan;
    }

    static constexpr int timer_slot() {
        auto cap = PIN_MAP[pin_map_index()];
        for (int i = 0; i < 4; i++) {
            if (cap.timers[i].timer_inst == 0xFF) break;
            return i;
        }
        return -1;
    }

    static constexpr Timer primary_timer() {
        int s = timer_slot();
        return s < 0 ? Timer(0xFF)
                     : static_cast<Timer>(PIN_MAP[pin_map_index()].timers[s].timer_inst);
    }

    static constexpr SercomInst primary_sercom() {
        auto cap = PIN_MAP[pin_map_index()];
        for (int i = 0; i < 4; i++) {
            if (cap.sercoms[i].sercom_inst == 0xFF) break;
            return static_cast<SercomInst>(cap.sercoms[i].sercom_inst);
        }
        return SercomInst(0xFF);
    }

    static constexpr uint8_t usb_func() {
        return PIN_MAP[pin_map_index()].usb_func;
    }

    // =====================================================================
    // GPIO: use_out() - Configure as digital output.
    // Returns the pin so calls can be chained: chip.PA07.use_out().set(1)
    // =====================================================================

    Pin& use_out() const {
        ResourceManager& rm = ResourceManager::instance();
        
        uint8_t idx = PinNum / 2;
        bool odd = PinNum & 1;
        
        if (rm.reserve_pmux(pin_map_index(), odd) != Status::OK) {
            return *const_cast<Pin*>(this);
        }

        volatile uint32_t* port_base = reinterpret_cast<volatile uint32_t*>(PortAddr);
        port_base[PORT_DIRSET_OFFSET >> 2] = (1UL << PinNum); // Set direction to output
        volatile uint8_t* pinconfig = reinterpret_cast<volatile uint8_t*>(PortAddr + PinNum + SAMD21_PORT_PIN_PAGE_SIZE);
        *pinconfig &= ~(PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN); // Disable PMUX and input

        return *const_cast<Pin*>(this);
    }

    // =====================================================================
    // GPIO: use_in() - Configure as digital input. Returns the pin for chaining.
    // =====================================================================

    Pin& use_in() const {
        ResourceManager& rm = ResourceManager::instance();
        
        uint8_t idx = PinNum / 2;
        bool odd = PinNum & 1;
        
        if (rm.reserve_pmux(pin_map_index(), odd) != Status::OK) {
            return *const_cast<Pin*>(this);
        }

        volatile uint32_t* port_base = reinterpret_cast<volatile uint32_t*>(PortAddr);
        port_base[PORT_OUTCLR_OFFSET >> 2] = (1UL << PinNum); // Clear output latch
        volatile uint8_t* pinconfig = reinterpret_cast<volatile uint8_t*>(PortAddr + PinNum + SAMD21_PORT_PIN_PAGE_SIZE);
        *pinconfig = PORT_PINCFG_INEN; // Enable input, PMUXEN already cleared by reserve

        return *const_cast<Pin*>(this);
    }

    // =====================================================================
    // ADC: use_adc() - Configure as analog input with compile-time check,
    // and bring up the shared ADC peripheral so adc_read() works.
    //
    // First call enables the APB clock, routes GCLK0 (48 MHz) to the ADC,
    // software-resets it and programs the fixed settings (1.0V internal
    // reference, 12-bit result, 1/512 prescaler, 63 ADC-clock sampling).
    // The SAMD21 has a single ADC: calling use_adc() on another pin later
    // re-points the input mux there (last pin wins). Returns the pin for
    // chaining: chip.PA05.use_adc().adc_read()
    // =====================================================================

    Pin& use_adc() const {
        static_assert(adc_channel() != AdcChan::NONE,
            "Pin does not support ADC. Use a different pin (e.g., PA02 for AIN0).");

        ResourceManager& rm = ResourceManager::instance();
        const bool first_use = (rm.reserve_adc() == Status::OK);

        configure_adc(adc_channel(), first_use);

        volatile uint8_t* pinconfig = reinterpret_cast<volatile uint8_t*>(PortAddr + PinNum + SAMD21_PORT_PIN_PAGE_SIZE);
        *pinconfig = PORT_PINCFG_PMUXEN;
        
        uint8_t idx = PinNum / 2;
        volatile uint8_t* pmux = reinterpret_cast<volatile uint8_t*>(PortAddr + idx + SAMD21_PORT_PMUX_OFFSET);
        if (PinNum & 1) {
            *pmux = (*pmux & 0x0F) | (PORT_PMUX_PMUXE_B_Val << 4);
        } else {
            *pmux = (*pmux & 0xF0) | PORT_PMUX_PMUXE_B_Val;
        }

        return *const_cast<Pin*>(this);
    }

    // =====================================================================
    // PWM: use_pwm() - Configure the pin as a PWM output and set the PWM
    // period on its primary timer. Duty is set separately with duty()
    // (safe to call repeatedly at runtime, glitch-free on TCC).
    // freq_hz == 0 selects the timer's maximum period. Returns the pin for
    // chaining: chip.PA08.use_pwm(150470).duty(0.0f)
    // =====================================================================

    Pin& use_pwm(uint32_t freq_hz) const {
        static_assert(primary_timer() != Timer(0xFF),
            "Pin does not support PWM. Use a different pin with timer capability.");

        ResourceManager& rm = ResourceManager::instance();

        if (rm.reserve_pmux(pin_map_index(), PinNum & 1) != Status::OK) {
            return *const_cast<Pin*>(this);
        }

        constexpr Timer t = primary_timer();
        const uint8_t tval = static_cast<uint8_t>(t);

        enable_timer_clock(t); // before writing any timer register
        if (rm.reserve_timer(t) != Status::OK) {
            rm.release_pmux(pin_map_index(), PinNum & 1);
            return *const_cast<Pin*>(this);
        }
        enable_timer_gclk(t);  // route GCLK0 (48 MHz) to the timer's channel

        // Mux the pin to this timer output's function (E or F, per pin map).
        volatile uint8_t* pinconfig = reinterpret_cast<volatile uint8_t*>(PortAddr + PinNum + SAMD21_PORT_PIN_PAGE_SIZE);
        *pinconfig = PORT_PINCFG_PMUXEN;

        constexpr int slot = timer_slot();
        const auto& tcap = PIN_MAP[pin_map_index()].timers[slot];
        const uint8_t pmux_val = tcap.pmux;
        const uint8_t wo = tcap.output;
        const uint8_t idx = PinNum / 2;
        volatile uint8_t* pmux = reinterpret_cast<volatile uint8_t*>(PortAddr + idx + SAMD21_PORT_PMUX_OFFSET);
        if (PinNum & 1) {
            *pmux = (*pmux & 0x0F) | (pmux_val << 4);
        } else {
            *pmux = (*pmux & 0xF0) | pmux_val;
        }

        // Period: counter cycles 0..top, so PER = gclk/freq - 1 (48 MHz GCLK0).
        uint32_t top;
        if (freq_hz > 0 && freq_hz <= 48000000UL) {
            top = (48000000UL / freq_hz) - 1;
        } else {
            top = 0xFFFFFFFFu; // clamped to the counter width below
        }

        if (tval <= 2) {
            Tcc* tccs[] = { TCC0, TCC1, TCC2 };
            Tcc* tcc = tccs[tval];
            tcc->CTRLA.reg = TCC_CTRLA_SWRST;
            while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST) {}
            tcc->CTRLA.reg |= TCC_CTRLA_PRESCALER_DIV1;
            tcc->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM;
            while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_WAVE) {}
            tcc->PER.reg = top & 0xFFFF;
            while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_PER) {}
            tcc->CC[wo].reg = 0;
            while (tcc->SYNCBUSY.reg & (TCC_SYNCBUSY_CC0 | TCC_SYNCBUSY_CC1 |
                                        TCC_SYNCBUSY_CC2 | TCC_SYNCBUSY_CC3)) {}
            tcc->CTRLA.reg |= TCC_CTRLA_ENABLE;
            while (tcc->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE) {}
        } else {
            Tc* tcs[] = { TC3, TC4, TC5 };
            Tc* tc = tcs[tval - 4];
            tc->COUNT8.CTRLA.reg = TC_CTRLA_SWRST;
            while (tc->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {}
            tc->COUNT8.CTRLA.reg = TC_CTRLA_MODE_COUNT8
                                 | TC_CTRLA_WAVEGEN_NPWM
                                 | TC_CTRLA_PRESCALER_DIV1;
            tc->COUNT8.PER.reg = top & 0xFF;
            tc->COUNT8.CC[wo & 1].reg = 0;
            while (tc->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {}
            tc->COUNT8.CTRLA.reg |= TC_CTRLA_ENABLE;
        }

        return *const_cast<Pin*>(this);
    }

    // =====================================================================
    // PWM: duty() - Set the duty cycle (0.0 .. 1.0) of the pin's timer
    // output. Runtime-safe and glitch-free on TCC (value is buffered and
    // transferred to the compare register on the next UPDATE event). On
    // 8-bit TC (no compare buffer) the compare register is written
    // directly. The timer must already be running (see use_pwm()).
    // Returns the pin for chaining.
    // =====================================================================

    Pin& duty(float d) const {
        static_assert(primary_timer() != Timer(0xFF),
            "Pin does not support PWM. Use a different pin with timer capability.");

        const float clamped = (d < 0.0f) ? 0.0f : (d > 1.0f) ? 1.0f : d;

        constexpr int slot = timer_slot();
        const uint8_t wo = PIN_MAP[pin_map_index()].timers[slot].output;
        const uint8_t tval = static_cast<uint8_t>(primary_timer());
        const uint32_t top = (tval <= 2) ? 0xFFFFu : 0xFFu;
        const uint32_t cmp = static_cast<uint32_t>(clamped * (float)top);

        if (tval <= 2) {
            Tcc* tccs[] = { TCC0, TCC1, TCC2 };
            Tcc* tcc = tccs[tval];
            tcc->CCB[wo].reg = cmp;
            tcc->CTRLBSET.reg = TCC_CTRLBSET_CMD_UPDATE; // synchronized, glitch-free
        } else {
            Tc* tcs[] = { TC3, TC4, TC5 };
            Tc* tc = tcs[tval - 4];
            tc->COUNT8.CC[wo & 1].reg = static_cast<uint8_t>(cmp);
        }
        return *const_cast<Pin*>(this);
    }

    // =====================================================================
    // SERCOM: use_sercom() - Configure as UART/SPI/I2C with compile-time
    // check. Returns the pin for chaining.
    // =====================================================================

    template <Peripheral P>
    Pin& use_sercom(SercomInst forced = SercomInst(0xFF)) const {
        static_assert(P == Peripheral::UART || P == Peripheral::SPI || P == Peripheral::IIC,
            "use_sercom() requires UART, SPI, or IIC peripheral type.");

        static_assert(primary_sercom() != SercomInst(0xFF),
            "Pin does not support any SERCOM (UART/SPI/I2C). Use a different pin.");

        ResourceManager& rm = ResourceManager::instance();
        
        SercomInst chosen = forced;
        if (chosen == SercomInst(0xFF)) {
            auto cap = PIN_MAP[pin_map_index()];
            for (int i = 0; i < 4; i++) {
                if (cap.sercoms[i].sercom_inst == 0xFF) break;
                if (rm.is_sercom_available(static_cast<SercomInst>(cap.sercoms[i].sercom_inst)) == Status::OK) {
                    chosen = static_cast<SercomInst>(cap.sercoms[i].sercom_inst);
                    break;
                }
            }
            if (chosen == SercomInst(0xFF) && PIN_MAP[pin_map_index()].sercoms[0].sercom_inst != 0xFF) {
                chosen = static_cast<SercomInst>(PIN_MAP[pin_map_index()].sercoms[0].sercom_inst);
            }
        }

        // Array lookup keyed by integer value - avoids CMSIS macro expansion in switch labels
        constexpr uint32_t kSercomPmMask[] = {
            PM_APBCMASK_SERCOM0,
            PM_APBCMASK_SERCOM1,
            PM_APBCMASK_SERCOM2,
            PM_APBCMASK_SERCOM3,
        };
        uint8_t idx = static_cast<uint8_t>(chosen);
        uint32_t mask = (idx < 4) ? kSercomPmMask[idx] : 0;
        
        if (mask) enable_clock(mask);
        // The SERCOM may already be reserved by the peripheral object that
        // requested this pin (e.g. the I2C wrapper reserves the SERCOM, then
        // muxes both SCL and SDA pins onto it). Reserve if free; configure
        // the pin either way since several pins share one SERCOM.
        if (rm.is_sercom_available(chosen) == Status::OK) {
            rm.reserve_sercom(chosen);
        }

        volatile uint8_t* pinconfig = reinterpret_cast<volatile uint8_t*>(PortAddr + PinNum + SAMD21_PORT_PIN_PAGE_SIZE);
        *pinconfig = PORT_PINCFG_PMUXEN;
        
        uint8_t idx2 = PinNum / 2;
        
        // SAMD21 SERCOM: Function C for pins 16-31, Function D for pins 0-15
        uint8_t pmux_val = (PinNum >= 16) ? PORT_PMUX_PMUXE_C_Val : PORT_PMUX_PMUXE_D_Val;
        
        volatile uint8_t* pmux_reg = reinterpret_cast<volatile uint8_t*>(PortAddr + SAMD21_PORT_PMUX_OFFSET + (PinNum / 2));
        if (PinNum & 1) {
            *pmux_reg = (*pmux_reg & 0x0F) | (pmux_val << 4);
        } else {
            *pmux_reg = (*pmux_reg & 0xF0) | pmux_val;
        }

        return *const_cast<Pin*>(this);
    }

    // =====================================================================
    // USB: use_usb() - Configure as USB DM/DP (PA24/PA25 only) with
    // compile-time check. Returns the pin for chaining.
    // =====================================================================

    Pin& use_usb() const {
        static_assert(PortAddr == PORT_IOBUS_BASE_ADDR && (PinNum == 24 || PinNum == 25),
            "USB requires PA24 (DM) or PA25 (DP). Use chip.PA24.use_usb() and chip.PA25.use_usb().");

        static_assert(usb_func() == PORT_PMUX_PMUXE_F_Val,
            "Pin does not support USB. Only PA24/PA25 are valid for USB DM/DP.");

        ResourceManager& rm = ResourceManager::instance();

        uint8_t idx = PinNum / 2;
        bool odd = PinNum & 1;

        if (rm.reserve_pmux(pin_map_index(), odd) != Status::OK) {
            return *const_cast<Pin*>(this);
        }

        volatile uint8_t* pinconfig = reinterpret_cast<volatile uint8_t*>(PortAddr + PinNum + SAMD21_PORT_PIN_PAGE_SIZE);
        *pinconfig = PORT_PINCFG_PMUXEN;

        volatile uint8_t* pmux = reinterpret_cast<volatile uint8_t*>(PortAddr + idx + SAMD21_PORT_PMUX_OFFSET);
        if (odd) {
            *pmux = (*pmux & 0x0F) | (PORT_PMUX_PMUXE_F_Val << 4);
        } else {
            *pmux = (*pmux & 0xF0) | PORT_PMUX_PMUXE_F_Val;
        }

        if constexpr (PinNum == 25) {
            volatile uint8_t* pinconfig2 = reinterpret_cast<volatile uint8_t*>(PortAddr + PinNum + SAMD21_PORT_PIN_PAGE_SIZE);
            *pinconfig2 = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
        }

        return *const_cast<Pin*>(this);
    }

    // =====================================================================
    // GPIO operations
    // =====================================================================

    Pin& set(uint8_t val) const {
        volatile uint32_t* port_base = reinterpret_cast<volatile uint32_t*>(PortAddr);
        if (val) port_base[PORT_OUTSET_OFFSET >> 2] = (1UL << PinNum);
        else     port_base[PORT_OUTCLR_OFFSET >> 2] = (1UL << PinNum);
        return *const_cast<Pin*>(this);
    }

    Pin& toggle() const {
        volatile uint32_t* port_base = reinterpret_cast<volatile uint32_t*>(PortAddr);
        port_base[PORT_OUTTGL_OFFSET >> 2] = (1UL << PinNum);
        return *const_cast<Pin*>(this);
    }

    bool level() const {
        volatile uint32_t* port_in = reinterpret_cast<volatile uint32_t*>(PortAddr + PORT_IN_OFFSET); // IN at offset 0x20
        return (port_in[0] >> PinNum) & 1;
    }

    constexpr uint8_t pin_number() const { return PinNum; }

    // =====================================================================
    // ADC/DAC read operations
    // =====================================================================

    // Start a single conversion on the pin's ADC channel and block until
    // it completes. Returns the 12-bit result (0..4095). If the channel
    // was never selected (use_adc() not called) this just returns without
    // converting.
    uint16_t adc_read() const {
        static_assert(adc_channel() != AdcChan::NONE,
            "Pin does not support ADC. Call use_adc() on an analog pin first.");

        if ((ADC->CTRLA.reg & ADC_CTRLA_ENABLE) == 0) return 0;

        ADC->SWTRIG.bit.START = 1;
        while (!ADC->INTFLAG.bit.RESRDY) {}
        ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;  // W1C - clear data-ready flag
        return ADC->RESULT.reg;
    }

    void dac_write(uint16_t value) const {
        DAC->DATA.reg = value & 0xFFF;
    }

private:
    // =====================================================================
    // Clock and peripheral helpers
    // =====================================================================

    static void enable_clock(uint32_t mask) {
        PM->APBCMASK.reg |= mask;
    }

    static void enable_timer_clock(Timer t) {
        // Array indexed by Timer enum value (TCC0-3 = 0-3, TC3-5 = 4-6).
        // Index 3 (TCC3) is a dummy - the E18A has no TCC3.
        constexpr uint32_t kTimerPmMask[] = {
            PM_APBCMASK_TCC0, PM_APBCMASK_TCC1, PM_APBCMASK_TCC2, 0u, // TCC0-3
            PM_APBCMASK_TC3,  PM_APBCMASK_TC4,  PM_APBCMASK_TC5,   // TC3-5
        };
        const uint8_t val = static_cast<uint8_t>(t);
        enable_clock((val < 7) ? kTimerPmMask[val] : 0);
    }

    // Route GCLK0 (DFLL48M / 1 = 48 MHz, set up in Chip::init()) to the
    // timer counter's GCLK channel:
    //   TCC0/TCC1 -> 0x1A, TCC2/TC3 -> 0x1B, TC4/TC5 -> 0x1C
    static void enable_timer_gclk(Timer t) {
        // Array indexed by Timer enum value (TCC0-3 = 0-3, TC3-5 = 4-6).
        constexpr uint32_t kTimerGclkCh[] = {
            GCLK_CLKCTRL_ID_TCC0_TCC1, GCLK_CLKCTRL_ID_TCC0_TCC1,
            GCLK_CLKCTRL_ID_TCC2_TC3,  GCLK_CLKCTRL_ID_TCC2_TC3,
            GCLK_CLKCTRL_ID_TCC2_TC3,  GCLK_CLKCTRL_ID_TC4_TC5, GCLK_CLKCTRL_ID_TC4_TC5,
        };
        const uint8_t val = static_cast<uint8_t>(t);
        if (val >= 7) return;
        GCLK->CLKCTRL.reg = kTimerGclkCh[val] | GCLK_CLKCTRL_GEN(0) | GCLK_CLKCTRL_CLKEN;
        while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
    }

    static void enable_gclk(SercomInst s) {
        // Array lookup keyed by integer value - avoids CMSIS macro expansion in switch labels
        constexpr uint32_t kSercomPmMask[] = {
            PM_APBCMASK_SERCOM0,
            PM_APBCMASK_SERCOM1,
            PM_APBCMASK_SERCOM2,
            PM_APBCMASK_SERCOM3,
        };
        uint8_t idx = static_cast<uint8_t>(s);
        enable_clock((idx < 4) ? kSercomPmMask[idx] : 0);

        GCLK->STATUS.reg &= ~GCLK_STATUS_SYNCBUSY;
        GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID((static_cast<uint8_t>(s) + 20)) | 
                            GCLK_CLKCTRL_GEN(0) | 
                            GCLK_CLKCTRL_CLKEN;
    }

    // Bring up the (single shared) ADC peripheral and point its positive
    // mux at the given channel, leaving it enabled. On first use the clock
    // domains are set up, the peripheral is software-reset and the fixed
    // settings programmed (1.0V internal reference, 12-bit result, 1/512
    // prescaler, 63 ADC-clock sampling, negative input grounded). Later
    // calls only re-route the input mux - INPUTCTRL is write-synchronized,
    // so the new channel applies from the next conversion without stopping
    // the peripheral.
    static void configure_adc(AdcChan ch, bool first_use) {
        PM->APBCMASK.reg |= PM_APBCMASK_ADC;
        GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_ADC | GCLK_CLKCTRL_GEN(0) | GCLK_CLKCTRL_CLKEN;
        while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

        if (first_use) {
            ADC->CTRLA.bit.SWRST = 1;
            while (ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) {}
            ADC->REFCTRL.reg  = ADC_REFCTRL_REFSEL_INT1V;               // 1.0V internal reference
            ADC->CTRLB.reg    = ADC_CTRLB_PRESCALER_DIV512 | ADC_CTRLB_RESSEL_12BIT;
            ADC->SAMPCTRL.reg = 0x3F;                                   // 63 ADC-clock sampling
            ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_GAIN_1X;
        }

        ADC->INPUTCTRL.reg = (ADC->INPUTCTRL.reg & ~ADC_INPUTCTRL_MUXPOS_Msk)
                            | ADC_INPUTCTRL_MUXPOS(static_cast<uint8_t>(ch));

        if (first_use) {
            ADC->CTRLA.reg |= ADC_CTRLA_ENABLE;
            while (ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) {}

            // The first conversion after a reference change must not be used
            // (datasheet 33.5), so run and throw one away to let the
            // reference buffer settle.
            ADC->SWTRIG.bit.START = 1;
            while (!ADC->INTFLAG.bit.RESRDY) {}
            ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;
        }
    }
};
