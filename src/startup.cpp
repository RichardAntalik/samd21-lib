// Minimal stub implementations for bare-metal C++ runtime
#include <cstddef>
#include <cstdint>
#include "Chip.h"

// Weak application entry points — override these in your firmware code
__attribute__((weak)) void setup(void) {}
__attribute__((weak)) void loop(void) { for(;;); }

extern "C" {

// Declare symbols defined by the linker script
extern uint32_t _sidata; // Start of .data init values in FLASH
extern uint32_t _sdata;  // Start of .data in RAM
extern uint32_t _edata;  // End of .data in RAM
extern uint32_t _sbss;   // Start of .bss in RAM
extern uint32_t _ebss;   // End of .bss in RAM
extern uint32_t _estack; // Top of RAM

void Reset_Handler(void) {
    WDT->CTRL.reg = 0;
    while (WDT->STATUS.bit.SYNCBUSY);

    // Vector table
    SCB->VTOR = 0x2000;

    /* Set up stack pointer */
    //__set_MSP(reinterpret_cast<uint32_t>(&_estack));

    // Copy initialized global/static variables from FLASH to RAM
    uint32_t *src = &_sidata;
    uint32_t *dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }

    // Zero-out all uninitialized global/static variables (.bss) in RAM
    dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }

    // Call C++ global/static constructors from .init_array
    extern uint32_t __init_array_start[];
    extern uint32_t __init_array_end[];
    for (uint32_t* fn = __init_array_start; fn < __init_array_end; fn++) {
        ((void(*)())(*fn))();
    }

    chip.init();

    setup();
    for (;;) {
        loop();
    }
}

// Thread-safe static local variable support (Meyers singleton) - no locking needed in bare-metal
int __cxa_guard_acquire(unsigned long* guard) {
    return (*guard == 0) ? 1 : 0;
}

void __cxa_guard_release(unsigned long* guard) {
    *guard = 1;
}

void __cxa_guard_abort(unsigned long*) {}

// Minimal _exit for bare-metal (never returns)
void _exit(int status) {
    (void)status;
    for (;;) {}
}

// Heap support - minimal bump allocator starting after BSS memory
static char* heap_ptr = nullptr;

int _sbrk(int incr) {
    if (!heap_ptr) {
        heap_ptr = reinterpret_cast<char*>(&_ebss); // Start heap after global variables
    }
    char* prev = heap_ptr;
    heap_ptr += incr;
    return (intptr_t)prev;
}

// Syscall stubs - all return error for bare-metal
int _write(int fd, const void* buf, std::size_t count) {
    (void)fd; (void)buf; (void)count;
    return -1;
}

int _read(int fd, void* buf, std::size_t count) {
    (void)fd; (void)buf; (void)count;
    return -1;
}

int _close(int fd) {
    (void)fd;
    return -1;
}

int _fstat(int fd, void* stat) {
    (void)fd; (void)stat;
    return -1;
}

int _isatty(int fd) {
    (void)fd;
    return 0;
}

int _lseek(int fd, int offset, int whence) {
    (void)fd; (void)offset; (void)whence;
    return -1;
}

int _kill(int pid, int sig) {
    (void)pid; (void)sig;
    return -1;
}

int _getpid() {
    return -1;
}

// Switch-case emulation for Thumb-1
unsigned int __gnu_thumb1_case_uqi(unsigned int value, unsigned int table_addr) {
    // Minimal switch-case fallback - just return the value
    (void)value; (void)table_addr;
    return 0;
}

} // extern "C"
