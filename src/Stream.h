#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdarg>

// ============================================================================
// Stream - minimal Arduino-compatible base class
//
// Mirrors the Arduino Print/Stream interface (the subset that serial-style
// libraries rely on) so code written for Arduino - e.g. SerialConsole -
// compiles and runs unmodified on bare metal. Concrete transports (USBPrint,
// a future UART, ...) derive from this and implement the pure-virtual I/O.
//
// On Arduino the real <Stream.h> is used instead; this header is only pulled
// in on the bare-metal path (see SerialConsole.h).
// ============================================================================

class Stream {
public:
    virtual ~Stream() = default;

    // ---- Output (Arduino Print) ----
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
    virtual size_t print(const char *str) = 0;
    virtual size_t println(const char *str) = 0;
    virtual size_t println() = 0;

    // Numeric output - transport independent, so implemented once here on
    // top of write() (mirrors Arduino's Print class). Formats into a small
    // stack buffer and emits it with a single write().
    size_t print(float n, uint8_t decimals = 2) {
        if (decimals > 6) decimals = 6;
        char buf[32];
        int i = 0;
        bool neg = n < 0.0f;
        if (neg) buf[i++] = '-';
        float abs_n = n < 0 ? -n : n;
        unsigned long whole = static_cast<unsigned long>(abs_n);
        int digit_start = i;
        if (whole == 0) {
            buf[i++] = '0';
        } else {
            while (whole > 0) {
                buf[i++] = '0' + (whole % 10);
                whole /= 10;
            }
            for (int j = digit_start, k = i - 1; j < k; j++, k--) {
                char tmp = buf[j]; buf[j] = buf[k]; buf[k] = tmp;
            }
        }
        if (decimals > 0) {
            buf[i++] = '.';
            float frac = abs_n - static_cast<float>(whole);
            for (uint8_t d = 0; d < decimals; d++) {
                frac *= 10.0f;
                int digit = static_cast<int>(frac);
                if (digit > 9) digit = 9;
                buf[i++] = '0' + digit;
                frac -= digit;
            }
        }
        buf[i] = '\0';
        return write(reinterpret_cast<const uint8_t*>(buf), strlen(buf));
    }

    size_t println(float n, uint8_t decimals = 2) {
        size_t c = print(n, decimals);
        const char *nl = "\r\n";
        return c + write(reinterpret_cast<const uint8_t*>(nl), 2);
    }

    // ------------------------------------------------------------------
    // printf-style formatted output (simplified C subset)
    //
    // Specifiers:  %d %i  signed decimal   %u  unsigned decimal
    //              %x %X unsigned hex      %p  pointer (8 hex digits)
    //              %c  char                 %s  NUL-terminated string
    //              %f %lf  double, fixed decimal point
    //              %%  literal '%'
    // Flags:  '-' left-justify   '0' zero-fill   '+' force sign on %d
    // Both support optional width and .precision, e.g. "%5.2f". For %f
    // precision is the number of decimals (default 6; 0 prints an integer,
    // rounded). %s without a precision prints the whole string. 'h', 'hh',
    // 'l', 'll', 'z' are accepted and ignored - everything is 32-bit on
    // this target and floats promote to double in varargs.
    //
    // No heap, no FPU needed: numbers format into a stack buffer and are
    // emitted through write(), so any transport gets it for free.
    //
    // Rounding note: %f scales by 10^precision in double before rounding,
    // so at exact .5 boundaries the last digit may differ from a
    // correctly-rounded printf by one ulp of the input - irrelevant for
    // logging. Values above ~1.8e14 with a decimal point overflow and
    // print as 0 (never happens for sensor/log magnitudes).
    // ------------------------------------------------------------------

    size_t printf(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        size_t n = vprintf(fmt, ap);
        va_end(ap);
        return n;
    }

    size_t printlnf(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        size_t n = vprintf(fmt, ap);
        va_end(ap);
        const char* nl = "\r\n";
        return n + write(reinterpret_cast<const uint8_t*>(nl), 2);
    }

    size_t vprintf(const char* fmt, va_list ap) {
        static const char* HEX_LOWER = "0123456789abcdef";
        static const char* HEX_UPPER = "0123456789ABCDEF";

        char buf[64];
        size_t i = 0;
        size_t total = 0;

        auto flush = [&]() {
            if (i) { total += write(reinterpret_cast<const uint8_t*>(buf), i); i = 0; }
        };
        // Append bytes, flushing the buffer as it fills up.
        auto emit = [&](char c) {
            if (i == sizeof(buf)) flush();
            buf[i++] = c;
        };
        auto emit_str = [&](const char* s, size_t n) {
            while (n) { emit(*s); ++s; --n; }
        };

        char field[40];
        const char* f = fmt;

        while (*f) {
            if (*f != '%') {
                emit(*f++);
                continue;
            }
            ++f;

            // Flags
            bool left = false, zero = false, plus = false;
            while (*f == '-' || *f == '0' || *f == '+') {
                if (*f == '-') left = true;
                if (*f == '0') zero = true;
                if (*f == '+') plus = true;
                ++f;
            }

            // Width
            int width = 0;
            while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');

            // Precision
            int prec = -1;
            if (*f == '.') {
                ++f;
                prec = 0;
                while (*f >= '0' && *f <= '9') prec = prec * 10 + (*f++ - '0');
            }

            // Length modifier: h/hh/l/ll/z - all no-ops on this 32-bit target
            while (*f == 'h' || *f == 'l' || *f == 'z' || *f == 'L') ++f;

            char spec = *f++;
            char* p = field;
            bool numeric = true;

            switch (spec) {
            case 'd':
            case 'i': {
                int v = va_arg(ap, int);
                bool neg = v < 0;
                unsigned int a = neg ? -v : static_cast<unsigned int>(v);
                if (plus && !neg) *p++ = '+';
                if (neg) *p++ = '-';
                char tmp[16];
                int t = 0;
                do { tmp[t++] = '0' + (a % 10); a /= 10; } while (a);
                for (int k = t - 1; k >= 0; k--) *p++ = tmp[k];
                break;
            }
            case 'u': {
                unsigned int a = va_arg(ap, unsigned int);
                char tmp[16];
                int t = 0;
                do { tmp[t++] = '0' + (a % 10); a /= 10; } while (a);
                for (int k = t - 1; k >= 0; k--) *p++ = tmp[k];
                break;
            }
            case 'x':
            case 'X': {
                unsigned int a = va_arg(ap, unsigned int);
                const char* h = (spec == 'x') ? HEX_LOWER : HEX_UPPER;
                char tmp[16];
                int t = 0;
                do { tmp[t++] = h[a & 0xF]; a >>= 4; } while (a);
                for (int k = t - 1; k >= 0; k--) *p++ = tmp[k];
                break;
            }
            case 'p': {
                // 8 hex digits, most significant first
                unsigned long a = static_cast<unsigned long>(reinterpret_cast<std::uintptr_t>(va_arg(ap, void*)));
                char tmp[8];
                for (int k = 7; k >= 0; k--) { tmp[k] = HEX_LOWER[a & 0xF]; a >>= 4; }
                memcpy(field, tmp, 8);
                p = field + 8;
                break;
            }
            case 'c':
                numeric = false;
                *p++ = static_cast<char>(va_arg(ap, int));
                break;
            case 's': {
                numeric = false;
                const char* s = va_arg(ap, const char*);
                if (!s) s = "(null)";
                while (*s && (prec < 0 || static_cast<int>(p - field) < prec)) *p++ = *s++;
                break;
            }
            case 'f':
            case 'F': {
                double n = va_arg(ap, double);
                if (prec < 0 || prec > 10) prec = 6;
                const bool neg = n < 0;
                if (plus && !neg) *p++ = '+';
                double a = neg ? -n : n;
                if (prec == 0) {
                    unsigned long long d = static_cast<unsigned long long>(a + 0.5);
                    if (neg) *p++ = '-';
                    char tmp[24];
                    int t = 0;
                    do { tmp[t++] = '0' + (d % 10); d /= 10; } while (d);
                    for (int k = t - 1; k >= 0; k--) *p++ = tmp[k];
                } else {
                    unsigned long long scl = 1;
                    for (int k = 0; k < prec; k++) scl *= 10;
                    unsigned long long total_d = static_cast<unsigned long long>(a * static_cast<double>(scl) + 0.5);
                    if (neg) *p++ = '-';
                    unsigned long long ip = total_d / scl;
                    unsigned long long fp = total_d % scl;
                    char tmp[24];
                    int t = 0;
                    do { tmp[t++] = '0' + (ip % 10); ip /= 10; } while (ip);
                    for (int k = t - 1; k >= 0; k--) *p++ = tmp[k];
                    *p++ = '.';
                    char fpbuf[16];
                    for (int k = 0; k < prec; k++) { fpbuf[prec - 1 - k] = '0' + (fp % 10); fp /= 10; }
                    memcpy(p, fpbuf, prec);
                    p += prec;
                }
                break;
            }
            case '%':
                numeric = false;
                *p++ = '%';
                break;
            case '\0':
                flush(); // premature end of format string
                return total;
            default:
                numeric = false;
                *p++ = '%';
                *p++ = spec;
                break;
            }

            // Apply width / padding, then emit the field
            const int len = static_cast<int>(p - field);
            if (width > len) {
                const bool has_sign = (len > 0) && (field[0] == '-' || field[0] == '+');
                if (left) {
                    emit_str(field, len);
                    for (int k = 0; k < width - len; k++) emit(' ');
                } else if (zero && numeric && has_sign) {
                    // Sign first, then zeros: -0005
                    emit(field[0]);
                    for (int k = 0; k < width - len; k++) emit('0');
                    emit_str(field + 1, len - 1);
                } else {
                    const char padc = (zero && numeric) ? '0' : ' ';
                    for (int k = 0; k < width - len; k++) emit(padc);
                    emit_str(field, len);
                }
            } else {
                emit_str(field, len);
            }
        }
        flush();
        return total;
    }

    // ---- Input (Arduino Stream) ----
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() = 0;
};
