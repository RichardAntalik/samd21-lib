#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "UsbCDC.h"
#include "Stream.h"


// ============================================================================
// USBPrint - Arduino-style print/println helpers over CDC-ACM
// Wraps the interrupt-driven UsbCDC stack. User just calls chip.usb.print().
// No polling, no task() calls needed. Everything runs from USB ISR.
// ============================================================================

class USBPrint : public Stream {
public:
    size_t print(const char* str) override {
        if (!str) return 0;
        return getUsb().write(reinterpret_cast<const uint8_t*>(str), strlen(str));
    }

    size_t println(const char* str) override {
        size_t n = print(str);
        return n + getUsb().write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
    }

    size_t println() override {
        return getUsb().write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
    }

    size_t print(char c) {
        return getUsb().write(static_cast<uint8_t>(c));
    }

    size_t println(char c) {
        return print(c) + getUsb().write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
    }

    size_t print(int n) {
        char buf[13];
        if (n == 0) {
            buf[0] = '0'; buf[1] = '\0';
        } else {
            int i = 0;
            bool neg = n < 0;
            unsigned long abs_n = neg ? -((unsigned long)n) : (unsigned long)n;
            while (abs_n > 0) {
                buf[i++] = '0' + (abs_n % 10);
                abs_n /= 10;
            }
            if (neg) buf[i++] = '-';
            buf[i] = '\0';
            for (int j = 0, k = i - 1; j < k; j++, k--) {
                char tmp = buf[j]; buf[j] = buf[k]; buf[k] = tmp;
            }
        }
        return getUsb().write(reinterpret_cast<const uint8_t*>(buf), strlen(buf));
    }

    size_t println(int n) {
        return print(n) + getUsb().write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
    }

    size_t print(unsigned long n) {
        char buf[21];
        if (n == 0) {
            buf[0] = '0'; buf[1] = '\0';
        } else {
            int i = 0;
            while (n > 0) {
                buf[i++] = '0' + (n % 10);
                n /= 10;
            }
            buf[i] = '\0';
            for (int j = 0, k = i - 1; j < k; j++, k--) {
                char tmp = buf[j]; buf[j] = buf[k]; buf[k] = tmp;
            }
        }
        return getUsb().write(reinterpret_cast<const uint8_t*>(buf), strlen(buf));
    }

    size_t println(unsigned long n) {
        return print(n) + getUsb().write(reinterpret_cast<const uint8_t*>("\r\n"), 2);
    }

    // Note: print(float, uint8_t)/println(float, uint8_t) are provided by
    // the Stream base class (transport-independent formatting on write()).

    // Passthrough to underlying CDC serial
    size_t write(uint8_t c) override { return getUsb().write(c); }
    size_t write(const uint8_t *buffer, size_t size) override { return getUsb().write(buffer, size); }
    int available() override { return getUsb().available(); }
    int read() override { return getUsb().read(); }
    int peek() override { return getUsb().peek(); }
    void flush() override { getUsb().flush(); }
    bool ready() { return getUsb().configured(); }
    explicit operator bool() noexcept { return static_cast<bool>(getUsb()); }

private:
    USBSerial& getUsb() noexcept { return chip_usb_cdc; }
};
