#pragma once
#include <cstdint>

// Peripheral function (for use_as())
// Note: ADC/DAC/EIC/GCLK renamed to avoid CMSIS macro conflicts
enum class Peripheral {
    OUT,    // GPIO output
    IN,     // GPIO input
    PERIPH_ADC,  // Analog input (was ADC - conflicts with CMSIS #define ADC)
    PWM,    // Pulse width modulation
    IIC,    // I2C (auto-selects SERCOM)
    UART,   // USART (auto-selects SERCOM)
    SPI,    // SPI (auto-selects SERCOM)
    PERIPH_DAC,  // Digital-to-analog output (was DAC - conflicts with CMSIS #define DAC)
    PERIPH_EIC,  // External interrupt (was EIC - conflicts with CMSIS #define EIC)
    PERIPH_GCLK, // Generic clock output (was GCLK - conflicts with CMSIS #define GCLK)
};

// SERCOM instances (for override: .sercom(SERCOM3))
enum class SercomInst : uint8_t {
    INST_SERCOM0 = 0, INST_SERCOM1 = 1, INST_SERCOM2 = 2,
    INST_SERCOM3 = 3, INST_SERCOM4 = 4, INST_SERCOM5 = 5,
};

// Timer/Counter instances
enum class Timer : uint8_t {
    INST_TCC0 = 0, INST_TCC1 = 1, INST_TCC2 = 2, INST_TCC3 = 3,
    INST_TC3    = 4, INST_TC4 = 5, INST_TC5 = 6, INST_TC6 = 7, INST_TC7 = 8,
};

// ADC channels
enum class AdcChan : uint8_t {
    NONE = 0xFF,
    AIN0 = 0, AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7,
    AIN8, AIN9, AIN10, AIN11, AIN12, AIN13, AIN14, AIN15,
    AIN16, AIN17, AIN18, AIN19,
};

// Sleep modes
enum class SleepMode { IDLE_0, IDLE_1, IDLE_2, STANDBY };

// Status codes
enum class Status : uint8_t { OK, BUSY, INVALID };
