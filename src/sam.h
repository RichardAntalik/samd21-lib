#pragma once

// SAMD21 CMSIS wrapper for TinyUSB DCD driver
// Wraps Atmel CMSIS pack headers with additional definitions needed by TinyUSB

#include <stdint.h>

// Official CMSIS device header (provides IRQn_Type, component headers, base addresses)
#include "libs/cmsis/samd21a/include/samd21e18a.h"

// Interrupt numbers for SAMD21 (single USB instance)
#define USB_0_IRQn  USB_IRQn
#define USB_1_IRQn  ((IRQn_Type) -1) // Not present on SAMD21
#define USB_2_IRQn  ((IRQn_Type) -1) // Not present on SAMD21
#define USB_3_IRQn  ((IRQn_Type) -1) // Not present on SAMD21

// USB calibration data bit positions (from nvmctrl.h via CMSIS)
// USB_FUSES_TRANSP_ADDR, USB_FUSES_TRANSN_ADDR, USB_FUSES_TRIM_ADDR are defined in component/nvmctrl.h
// All three are at NVMCTRL_OTP4 + 4 with different bit offsets

// Number of USB endpoints in SAMD21
#define USB_EPT_NUM             8
