#pragma once
#include <cstdint>
#include <new>

#include "Types.h"

// ============================================================================
// ResourceManager - Static bitmask engine for peripheral occupancy tracking
//
// Prevents peripheral conflicts at runtime by tracking which resources are
// already in use. All state is static (no dynamic allocation).
// ============================================================================

class ResourceManager {
public:
    static constexpr uint8_t PMUX_MAX = 32;

    // Get singleton instance
    static ResourceManager& instance() {
        static ResourceManager rm;
        return rm;
    }

    // Check and reserve a SERCOM instance
    Status reserve_sercom(SercomInst sercom) {
        uint8_t bit = static_cast<uint8_t>(sercom);
        if (bit >= 6) return Status::INVALID;
        if (sercom_mask_ & (1U << bit)) {
            return Status::BUSY;
        }
        sercom_mask_ |= (1U << bit);
        return Status::OK;
    }

    // Release a SERCOM instance
    void release_sercom(SercomInst sercom) {
        uint8_t bit = static_cast<uint8_t>(sercom);
        if (bit < 6) {
            sercom_mask_ &= ~(1U << bit);
        }
    }

    // Check if a SERCOM instance is available (without reserving)
    Status is_sercom_available(SercomInst sercom) const {
        uint8_t bit = static_cast<uint8_t>(sercom);
        if (bit >= 6) return Status::INVALID;
        return !(sercom_mask_ & (1U << bit)) ? Status::OK : Status::BUSY;
    }

    // Check and reserve a timer instance
    Status reserve_timer(Timer timer) {
        uint8_t bit = static_cast<uint8_t>(timer);
        if (bit >= 9) return Status::INVALID;
        if (timer_mask_ & (1U << bit)) {
            return Status::BUSY;
        }
        timer_mask_ |= (1U << bit);
        return Status::OK;
    }

    // Release a timer instance
    void release_timer(Timer timer) {
        uint8_t bit = static_cast<uint8_t>(timer);
        if (bit < 9) {
            timer_mask_ &= ~(1U << bit);
        }
    }

    // Check if a timer instance is available (without reserving)
    bool is_timer_available(Timer timer) const {
        uint8_t bit = static_cast<uint8_t>(timer);
        if (bit >= 9) return false;
        return !(timer_mask_ & (1U << bit));
    }

    // Check and reserve ADC
    Status reserve_adc() {
        if (adc_mask_ & 1) {
            return Status::BUSY;
        }
        adc_mask_ = 1;
        return Status::OK;
    }

    // Release ADC
    void release_adc() {
        adc_mask_ = 0;
    }

    // Check and reserve DAC
    Status reserve_dac() {
        if (dac_mask_ & 1) {
            return Status::BUSY;
        }
        dac_mask_ = 1;
        return Status::OK;
    }

    // Release DAC
    void release_dac() {
        dac_mask_ = 0;
    }

    // Check and reserve a PMUX slot (pin-level multiplexing)
    Status reserve_pmux(uint8_t pin_index, bool is_odd) {
        if (pin_index >= PMUX_MAX) return Status::INVALID;
        uint16_t bit = pin_index * 2 + (is_odd ? 1 : 0);
        if (pmux_mask_ & (1U << bit)) {
            return Status::BUSY;
        }
        pmux_mask_ |= (1U << bit);
        return Status::OK;
    }

    // Release a PMUX slot
    void release_pmux(uint8_t pin_index, bool is_odd) {
        if (pin_index >= PMUX_MAX) return;
        uint16_t bit = pin_index * 2 + (is_odd ? 1 : 0);
        pmux_mask_ &= ~(1U << bit);
    }

    // Check if a PMUX slot is available
    bool is_pmux_available(uint8_t pin_index, bool is_odd) const {
        if (pin_index >= PMUX_MAX) return false;
        uint16_t bit = pin_index * 2 + (is_odd ? 1 : 0);
        return !(pmux_mask_ & (1U << bit));
    }

    // Get current SERCOM mask (for debugging)
    uint8_t get_sercom_mask() const { return sercom_mask_; }

    // Get current timer mask (for debugging)
    uint8_t get_timer_mask() const { return timer_mask_; }

    // Get current PMUX mask (for debugging)
    uint16_t get_pmux_mask() const { return pmux_mask_; }

    // Reset all resources (for testing/recovery)
    void reset_all() {
        sercom_mask_ = 0;
        timer_mask_ = 0;
        adc_mask_ = 0;
        dac_mask_ = 0;
        pmux_mask_ = 0;
    }

    // Check if any resource is in use
    bool any_in_use() const {
        return sercom_mask_ || timer_mask_ || adc_mask_ || dac_mask_ || pmux_mask_;
    }

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    uint8_t sercom_mask_;   // 6 bits: SERCOM0-SERCOM5
    uint8_t timer_mask_;    // 9 bits: TCC0-3, TC3-7
    uint8_t adc_mask_;      // 1 bit: ADC
    uint8_t dac_mask_;      // 1 bit: DAC
    uint16_t pmux_mask_;    // 64 bits (2 per pin x 32 pins): PMUX slots
};
