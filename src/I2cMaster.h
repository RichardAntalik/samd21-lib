#pragma once
#include <cstdint>
#include <cstring>

// CMSIS headers for SAMD21 (provides GCLK, PM, Sercom types)
#include "libs/cmsis/Core/Include/cmsis_gcc.h"
#include "libs/cmsis/samd21a/include/samd21e18a.h"

#include "Types.h"
#include "ResourceManager.h"
#include "Pin.h"

// ============================================================================
// I2cMaster<S> - I2C Master implementation for SERCOM peripheral
//
// Template parameter S selects which SERCOM instance to use. Each instantiation
// is a distinct type with its own static state (zero-cost, no dynamic alloc).
//
// All operations are blocking with timeout. Returns Status::OK on success.
// Uses polling (no interrupts) — ISR support planned for future.
//
// Usage:
//   chip.PA16.use_sercom<Peripheral::IIC>();  // SCL pad2
//   chip.PA17.use_sercom<Peripheral::IIC>();  // SDA pad3
//   auto& i2c = chip.i2c(SercomInst::INST_SERCOM0);
//   i2c.init(100000);                         // 100 kHz
//   i2c.send(0x68, data, len);                // write to device
//   i2c.receive(0x68, buf, len);              // read from device
// ============================================================================

template <SercomInst S>
class I2cMaster {
public:
    static constexpr uint8_t SERCOM_INDEX = static_cast<uint8_t>(S);

    static_assert(SERCOM_INDEX <= 5, "I2C: SERCOM instance out of range for this chip variant");

    I2cMaster() : sercom_(get_sercom_ptr(S)), init_done_(false), timeout_cycles_(DEFAULT_TIMEOUT_CYCLES) {}

    static I2cMaster& instance() {
        static I2cMaster inst;
        return inst;
    }

    void init(uint32_t clock_hz = 100000) {
        setup_sercom_clock(SERCOM_INDEX);

        Sercom* s = sercom_;

        // Software reset
        s->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_SWRST;
        while (s->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SWRST) {}

        // Configure CTRLA: MODE = I2C Master, SDAHOLD enabled
        uint32_t ctrla = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER |
                         SERCOM_I2CM_CTRLA_SDAHOLD(1);
        s->I2CM.CTRLA.reg = ctrla;
        wait_sync(s);

        // Set baud rate
        s->I2CM.BAUD.reg = i2c_baud(clock_hz);
        wait_sync(s);

        // Enable peripheral
        s->I2CM.CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE;
        wait_sync(s);

        // Force bus state to IDLE via SYNCBUSY.SYSOP
        s->I2CM.STATUS.reg = SERCOM_I2CM_STATUS_BUSSTATE(1);
        while (s->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_SYSOP) {}

        init_done_ = true;
    }

    Status send(uint8_t addr, uint8_t data) {
        if (!init_done_) return Status::BUSY;
        if (i2c_send_addr(addr, false) != Status::OK) {
            i2c_stop();
            return Status::INVALID;
        }
        i2c_write_byte(data);
        i2c_stop();
        return Status::OK;
    }

    Status send(uint8_t addr, const uint8_t* data, size_t len) {
        if (!init_done_) return Status::BUSY;
        if (len == 0) return Status::OK;

        if (i2c_send_addr(addr, false) != Status::OK) {
            i2c_stop();
            return Status::INVALID;
        }

        for (size_t i = 0; i < len; i++) {
            i2c_write_byte(data[i]);
        }
        i2c_stop();
        return Status::OK;
    }

    Status receive(uint8_t addr, uint8_t& data) {
        if (!init_done_) return Status::BUSY;
        if (i2c_send_addr(addr, true) != Status::OK) {
            i2c_stop();
            return Status::INVALID;
        }
        data = i2c_read_byte(true); // NACK after last byte
        i2c_stop();
        return Status::OK;
    }

    Status receive(uint8_t addr, uint8_t* buf, size_t len) {
        if (!init_done_) return Status::BUSY;
        if (len == 0) return Status::OK;

        if (i2c_send_addr(addr, true) != Status::OK) {
            i2c_stop();
            return Status::INVALID;
        }

        for (size_t i = 0; i < len; i++) {
            bool is_last = (i == len - 1);
            buf[i] = i2c_read_byte(is_last);
        }
        i2c_stop();
        return Status::OK;
    }

    int scan(uint8_t* addrs, uint8_t max_count = 126) {
        if (!init_done_) return 0;
        int count = 0;
        for (uint8_t addr = 1; addr < 127 && count < (int)max_count; addr++) {
            if (i2c_send_addr(addr, false) == Status::OK) {
                addrs[count++] = addr;
            }
            i2c_stop(); // Issues STOP to release the bus for the next probe
        }
        return count;
    }

    void release() {
        Sercom* s = sercom_;
        s->I2CM.CTRLA.reg &= ~SERCOM_I2CM_CTRLA_ENABLE;
        wait_sync(s);
        init_done_ = false;

        ResourceManager& rm = ResourceManager::instance();
        rm.release_sercom(S);
    }

    void set_timeout(uint32_t cycles) { timeout_cycles_ = cycles; }
    bool is_initialized() const { return init_done_; }

private:
    static constexpr uint32_t DEFAULT_TIMEOUT_CYCLES = 240000;
    Sercom* sercom_;

    static void setup_sercom_clock(uint8_t idx) {
        static constexpr uint32_t kPmMask[] = { PM_APBCMASK_SERCOM0, PM_APBCMASK_SERCOM1, PM_APBCMASK_SERCOM2, PM_APBCMASK_SERCOM3 };
        static constexpr uint32_t kGclkId[] = { SERCOM0_GCLK_ID_CORE, SERCOM1_GCLK_ID_CORE, SERCOM2_GCLK_ID_CORE, SERCOM3_GCLK_ID_CORE };

        if (idx < 4) {
            PM->APBCMASK.reg |= kPmMask[idx];
            GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(kGclkId[idx]) | GCLK_CLKCTRL_GEN(GCLK_CLKCTRL_GEN_GCLK0_Val) | GCLK_CLKCTRL_CLKEN;
            while (GCLK->STATUS.bit.SYNCBUSY) {}
        } else if (idx == 4) {
#if defined(SERCOM4_BASE_ADDR) || defined(SERCOM4_REGS)
            PM->APBCMASK.reg |= PM_APBCMASK_SERCOM4;
            GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SERCOM4_GCLK_ID_CORE) | GCLK_CLKCTRL_GEN(GCLK_CLKCTRL_GEN_GCLK0_Val) | GCLK_CLKCTRL_CLKEN;
            while (GCLK->STATUS.bit.SYNCBUSY) {}
#endif
        } else if (idx == 5) {
#if defined(SERCOM5_BASE_ADDR) || defined(SERCOM5_REGS)
            PM->APBCMASK.reg |= PM_APBCMASK_SERCOM5;
            GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SERCOM5_GCLK_ID_CORE) | GCLK_CLKCTRL_GEN(GCLK_CLKCTRL_GEN_GCLK0_Val) | GCLK_CLKCTRL_CLKEN;
            while (GCLK->STATUS.bit.SYNCBUSY) {}
#endif
        }
    }

    static Sercom* get_sercom_ptr(SercomInst s) {
        switch (static_cast<uint8_t>(s)) {
            case 0: return SERCOM0;
            case 1: return SERCOM1;
            case 2: return SERCOM2;
            case 3: return SERCOM3;
#if defined(SERCOM4_BASE_ADDR) || defined(SERCOM4_REGS)
            case 4: return SERCOM4;
#endif
#if defined(SERCOM5_BASE_ADDR) || defined(SERCOM5_REGS)
            case 5: return SERCOM5;
#endif
            default: return SERCOM0; // Should never happen due to static_assert
        }
    }

    // DA1 clock generation with BAUDLOW = 0: fSCL = fGCLK / (10 + 2 * BAUD)
    // => BAUD = (fGCLK / fSCL - 10) / 2
    static uint32_t i2c_baud(uint32_t clock_hz) {
        constexpr uint32_t PERIPHERAL_CLOCK = 48000000;
        if (clock_hz == 0) return 0;
        uint32_t div = PERIPHERAL_CLOCK / clock_hz;
        if (div <= 10) return 0;
        uint32_t baud = (div - 10) / 2;
        if (baud > SERCOM_I2CM_BAUD_BAUD_Msk) baud = SERCOM_I2CM_BAUD_BAUD_Msk;
        return baud;
    }

    static void wait_sync(Sercom* s) {
        while (s->I2CM.SYNCBUSY.reg & (SERCOM_I2CM_SYNCBUSY_SWRST | SERCOM_I2CM_SYNCBUSY_ENABLE)) {}
    }

    Status wait_tx_complete(Sercom* s) {
        uint32_t cycles = 0;
        while (!(s->I2CM.INTFLAG.reg & (SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB))) {
            if (++cycles >= timeout_cycles_) return Status::BUSY;
        }
        return Status::OK;
    }

    void i2c_stop() {
        sercom_->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(3) | SERCOM_I2CM_CTRLB_ACKACT;
    }

    Status i2c_send_addr(uint8_t addr_7bit, bool is_read) {
        uint8_t addr_byte = (addr_7bit << 1) | (is_read ? 1 : 0);
        sercom_->I2CM.ADDR.reg = addr_byte;

        if (wait_tx_complete(sercom_) != Status::OK) return Status::BUSY;

        uint8_t status = sercom_->I2CM.STATUS.reg;
        return (status & SERCOM_I2CM_STATUS_RXNACK) ? Status::INVALID : Status::OK;
    }

    void i2c_write_byte(uint8_t data) {
        sercom_->I2CM.DATA.reg = data;
        wait_tx_complete(sercom_);
    }

    uint8_t i2c_read_byte(bool is_last_byte) {
        uint32_t ctrlb = SERCOM_I2CM_CTRLB_CMD(2); // CMD = Read
        if (is_last_byte) {
            ctrlb |= SERCOM_I2CM_CTRLB_ACKACT; // NACK on last byte
        }
        sercom_->I2CM.CTRLB.reg = ctrlb;

        wait_tx_complete(sercom_);

        return sercom_->I2CM.DATA.reg;
    }

    bool init_done_;
    uint32_t timeout_cycles_;
};

// ============================================================================
// I2C - Arduino-style wrapper for non-template usage
//
// Usage patterns:
//   I2C i2c(SercomInst::INST_SERCOM0, 400000);     // SERCOM0, init at 400kHz
//   I2C i2c(SercomInst::INST_SERCOM0);              // SERCOM0, call init() later
//   auto i2c = I2C::make(chip.PA16, chip.PA17);     // Auto-detect SERCOM from pins
//   auto i2c = I2C::make(chip.PA16, chip.PA17, 400000);  // + init at 400kHz
// ============================================================================

class I2C {
public:
    // Construct with explicit SERCOM index and optional frequency (lazy init on first operation)
    explicit I2C(SercomInst sercom_idx, uint32_t freq = 0)
        : sercom_index_(sercom_idx), init_done_(false), init_freq_(freq > 0 ? freq : 100000) {
        ResourceManager& rm = ResourceManager::instance();
        if (rm.reserve_sercom(sercom_idx) != Status::OK) {
            // Already in use — still allow initialization for re-entry
        }
    }

    // Construct from Pin instances - auto-detects SERCOM via primary_sercom()
    template <uint32_t SCL_PORT, uint8_t SCL_NUM, uint32_t SDA_PORT, uint8_t SDA_NUM>
    I2C(const Pin<SCL_PORT, SCL_NUM>& scl_pin,
        const Pin<SDA_PORT, SDA_NUM>& sda_pin,
        uint32_t freq = 0) {
        static_assert(Pin<SCL_PORT, SCL_NUM>::primary_sercom() != SercomInst(0xFF), "SCL pin does not support SERCOM (I2C). Use a different pin.");
        static_assert(Pin<SDA_PORT, SDA_NUM>::primary_sercom() != SercomInst(0xFF), "SDA pin does not support SERCOM (I2C). Use a different pin.");
        static_assert(Pin<SCL_PORT, SCL_NUM>::primary_sercom() == Pin<SDA_PORT, SDA_NUM>::primary_sercom(),
            "SCL and SDA pins must be on the same SERCOM instance. Check your pin selection.");

        SercomInst sercom = scl_pin.primary_sercom();
        if (sercom == SercomInst(0xFF)) {
            sercom = sda_pin.primary_sercom();
        }
        this->sercom_index_ = sercom;

        ResourceManager& rm = ResourceManager::instance();
        if (rm.reserve_sercom(sercom) != Status::OK) {
            // Already in use — still allow initialization for re-entry
        }

        // Auto-configure pins as IIC (no forced SERCOM)
        scl_pin.template use_sercom<Peripheral::IIC>(sercom);
        sda_pin.template use_sercom<Peripheral::IIC>(sercom);

        this->init_freq_ = freq > 0 ? freq : 100000;
        this->init_done_= false;
        this->scl_pin = scl_pin.pin_number(); //XXX just for testing
    }

    void init(uint32_t clock_hz = 100000) {
        this->dispatch_init(clock_hz);
        init_done_ = true;
    }

    Status send(uint8_t addr, uint8_t data) {
        ensure_init();
        return this->dispatch_send(addr, data);
    }

    Status send(uint8_t addr, const uint8_t* data, size_t len) {
        ensure_init();
        return this->dispatch_send_buf(addr, data, len);
    }

    Status receive(uint8_t addr, uint8_t& data) {
        ensure_init();
        return this->dispatch_receive(addr, data);
    }

    Status receive(uint8_t addr, uint8_t* buf, size_t len) {
        ensure_init();
        return this->dispatch_receive_buf(addr, buf, len);
    }

    int scan(uint8_t* addrs, uint8_t max_count = 126) {
        ensure_init();
        return this->dispatch_scan(addrs, max_count);
    }

    void release() {
        ResourceManager& rm = ResourceManager::instance();
        rm.release_sercom(sercom_index_);
        init_done_ = false;
    }

private:
    SercomInst sercom_index_;
    bool init_done_;
    uint32_t init_freq_;
    uint8_t scl_pin = 0;

    void ensure_init() {
        if (init_done_) return;
        dispatch_init(init_freq_);
        init_done_ = true;
    }

    Status dispatch_init(uint32_t clock_hz) {
        switch (sercom_index_) {
            case SercomInst::INST_SERCOM0: I2cMaster<SercomInst::INST_SERCOM0>::instance().init(clock_hz); break;
            case SercomInst::INST_SERCOM1: I2cMaster<SercomInst::INST_SERCOM1>::instance().init(clock_hz); break;
            case SercomInst::INST_SERCOM2: I2cMaster<SercomInst::INST_SERCOM2>::instance().init(clock_hz); break;
            case SercomInst::INST_SERCOM3: I2cMaster<SercomInst::INST_SERCOM3>::instance().init(clock_hz); break;
#if defined(SERCOM4_BASE_ADDR) || defined(SERCOM4_REGS)
            case SercomInst::INST_SERCOM4: I2cMaster<SercomInst::INST_SERCOM4>::instance().init(clock_hz); break;
#endif
#if defined(SERCOM5_BASE_ADDR) || defined(SERCOM5_REGS)
            case SercomInst::INST_SERCOM5: I2cMaster<SercomInst::INST_SERCOM5>::instance().init(clock_hz); break;
#endif
        }
        return Status::OK;
    }

    Status dispatch_send(uint8_t addr, uint8_t data) {
        switch (sercom_index_) {
            case SercomInst::INST_SERCOM0: return I2cMaster<SercomInst::INST_SERCOM0>::instance().send(addr, data);
            case SercomInst::INST_SERCOM1: return I2cMaster<SercomInst::INST_SERCOM1>::instance().send(addr, data);
            case SercomInst::INST_SERCOM2: return I2cMaster<SercomInst::INST_SERCOM2>::instance().send(addr, data);
            case SercomInst::INST_SERCOM3: return I2cMaster<SercomInst::INST_SERCOM3>::instance().send(addr, data);
#if defined(SERCOM4_BASE_ADDR) || defined(SERCOM4_REGS)
            case SercomInst::INST_SERCOM4: return I2cMaster<SercomInst::INST_SERCOM4>::instance().send(addr, data);
#endif
#if defined(SERCOM5_BASE_ADDR) || defined(SERCOM5_REGS)
            case SercomInst::INST_SERCOM5: return I2cMaster<SercomInst::INST_SERCOM5>::instance().send(addr, data);
#endif
        }
        return Status::INVALID;
    }

    Status dispatch_send_buf(uint8_t addr, const uint8_t* data, size_t len) {
        switch (sercom_index_) {
            case SercomInst::INST_SERCOM0: return I2cMaster<SercomInst::INST_SERCOM0>::instance().send(addr, data, len);
            case SercomInst::INST_SERCOM1: return I2cMaster<SercomInst::INST_SERCOM1>::instance().send(addr, data, len);
            case SercomInst::INST_SERCOM2: return I2cMaster<SercomInst::INST_SERCOM2>::instance().send(addr, data, len);
            case SercomInst::INST_SERCOM3: return I2cMaster<SercomInst::INST_SERCOM3>::instance().send(addr, data, len);
#if defined(SERCOM4_BASE_ADDR) || defined(SERCOM4_REGS)
            case SercomInst::INST_SERCOM4: return I2cMaster<SercomInst::INST_SERCOM4>::instance().send(addr, data, len);
#endif
#if defined(SERCOM5_BASE_ADDR) || defined(SERCOM5_REGS)
            case SercomInst::INST_SERCOM5: return I2cMaster<SercomInst::INST_SERCOM5>::instance().send(addr, data, len);
#endif
        }
        return Status::INVALID;
    }

    Status dispatch_receive(uint8_t addr, uint8_t& data) {
        switch (sercom_index_) {
            case SercomInst::INST_SERCOM0: return I2cMaster<SercomInst::INST_SERCOM0>::instance().receive(addr, data);
            case SercomInst::INST_SERCOM1: return I2cMaster<SercomInst::INST_SERCOM1>::instance().receive(addr, data);
            case SercomInst::INST_SERCOM2: return I2cMaster<SercomInst::INST_SERCOM2>::instance().receive(addr, data);
            case SercomInst::INST_SERCOM3: return I2cMaster<SercomInst::INST_SERCOM3>::instance().receive(addr, data);
#if defined(SERCOM4_BASE_ADDR) || defined(SERCOM4_REGS)
            case SercomInst::INST_SERCOM4: return I2cMaster<SercomInst::INST_SERCOM4>::instance().receive(addr, data);
#endif
#if defined(SERCOM5_BASE_ADDR) || defined(SERCOM5_REGS)
            case SercomInst::INST_SERCOM5: return I2cMaster<SercomInst::INST_SERCOM5>::instance().receive(addr, data);
#endif
        }
        return Status::INVALID;
    }

    Status dispatch_receive_buf(uint8_t addr, uint8_t* buf, size_t len) {
        switch (sercom_index_) {
            case SercomInst::INST_SERCOM0: return I2cMaster<SercomInst::INST_SERCOM0>::instance().receive(addr, buf, len);
            case SercomInst::INST_SERCOM1: return I2cMaster<SercomInst::INST_SERCOM1>::instance().receive(addr, buf, len);
            case SercomInst::INST_SERCOM2: return I2cMaster<SercomInst::INST_SERCOM2>::instance().receive(addr, buf, len);
            case SercomInst::INST_SERCOM3: return I2cMaster<SercomInst::INST_SERCOM3>::instance().receive(addr, buf, len);
#if defined(SERCOM4_BASE_ADDR) || defined(SERCOM4_REGS)
            case SercomInst::INST_SERCOM4: return I2cMaster<SercomInst::INST_SERCOM4>::instance().receive(addr, buf, len);
#endif
#if defined(SERCOM5_BASE_ADDR) || defined(SERCOM5_REGS)
            case SercomInst::INST_SERCOM5: return I2cMaster<SercomInst::INST_SERCOM5>::instance().receive(addr, buf, len);
#endif
        }
        return Status::INVALID;
    }

    int dispatch_scan(uint8_t* addrs, uint8_t max_count) {
        switch (sercom_index_) {
            case SercomInst::INST_SERCOM0: return I2cMaster<SercomInst::INST_SERCOM0>::instance().scan(addrs, max_count);
            case SercomInst::INST_SERCOM1: return I2cMaster<SercomInst::INST_SERCOM1>::instance().scan(addrs, max_count);
            case SercomInst::INST_SERCOM2: return I2cMaster<SercomInst::INST_SERCOM2>::instance().scan(addrs, max_count);
            case SercomInst::INST_SERCOM3: return I2cMaster<SercomInst::INST_SERCOM3>::instance().scan(addrs, max_count);
#if defined(SERCOM4_BASE_ADDR) || defined(SERCOM4_REGS)
            case SercomInst::INST_SERCOM4: return I2cMaster<SercomInst::INST_SERCOM4>::instance().scan(addrs, max_count);
#endif
#if defined(SERCOM5_BASE_ADDR) || defined(SERCOM5_REGS)
            case SercomInst::INST_SERCOM5: return I2cMaster<SercomInst::INST_SERCOM5>::instance().scan(addrs, max_count);
#endif
        }
        return 0;
    }
};

// ============================================================================
// Future TODO: Callback-based async I2C
//
// When implementing interrupt-driven I2C, consider this architecture for
// handling multiple devices on the same bus with receive callbacks:
//
// 1. Add callback registration per device address:
//      using I2cReceiveCallback = void(uint8_t addr, uint8_t* data, size_t len);
//      template <SercomInst S>
//      void on_receive(I2cMaster<S>& i2c, uint8_t addr, I2cReceiveCallback cb);
//
// 2. Store "last register" context per device address:
//      struct DeviceContext {
//          uint8_t last_register;   // Last register written via send(addr, reg, data)
//          I2cReceiveCallback cb;
//          bool has_callback;
//      };
//      static inline DeviceContext device_contexts[128];  // Per-address state
//
// 3. ISR flow (SERCOM interrupt handler):
//      a. Check INTFLAG for MB (Master Bus) or SR (Slave Receive)
//      b. Read byte from DATA register into ring buffer
//      c. If device has registered callback, schedule it (set flag, main thread polls)
//      d. Callback receives: device_addr + data_buffer + last_register context
//
// 4. Main thread pattern:
//      if (i2c.has_pending(addr)) {
//          uint8_t reg = i2c.last_register(addr);
//          uint8_t buf[32];
//          size_t len = i2c.read_pending(addr, buf, sizeof(buf));
//          my_callback(addr, buf, len, reg);  // User callback with full context
//      }
//
// 5. Complex scenario — register write followed by read:
//      i2c.send_with_reg(0x68, 0x3B, &val, 1);  // Write register address
//      // At this point, last_register[0x68] = 0x3B
//      // When slave responds with data, callback knows it's reading from reg 0x3B
//
// Key challenges:
// - Multiple masters on same bus (arbitration handled by hardware)
// - Callback reentrancy — ISR fires while main thread is in another I2C op
//   Solution: ring buffer + flag, callback runs in main context
// - Address collisions — two devices with same address? User handles this.
// - Timeout handling in ISR — need watchdog or timeout counter
// ============================================================================
