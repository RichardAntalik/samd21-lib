# samd21-lib

Bare-metal C++17 framework for the Microchip SAMD21 (SAMD21E18A). This repo
**is** the chip library (`src/` plus its build/tools) - applications are
separate projects created with `tools/new/samd-new`. If a `main.cpp` is
dropped into the repo root it is additionally built as a firmware app
(convenient scratch bench); without it `make` builds the library only.

Target chip: Cortex-M0+ @ 48 MHz (DFLL 48M on GCLK0/GCLK1), 256 KB flash,
32 KB SRAM, no FPU. Boards flashed here are Adafruit Trinket-style (USB
serial + SAM-BA bootloader).

## Features

### Pins (compile-time capability checks)

Each pin is a unique type, so invalid pin/peripheral combinations fail
compilation with a `static_assert` message - no runtime surprises, no pin
tables in user code. Calls return the pin, so member calls can be chained:

```cpp
chip.PA07.use_out().set(1);                    // digital output
chip.PA10.use_out().toggle();
chip.PA03.use_in().level();                    // digital input
chip.PA08.use_pwm(150470).duty(0.4f);          // PWM + duty (0.0..1.0)
chip.PA05.use_adc().adc_read();                // 12-bit ADC (0..4095)
chip.PA16.use_sercom<Peripheral::IIC>();       // UART/SPI/I2C pad
chip.PA24.use_usb();                           // USB DM/DP (PA24/PA25 only)
```

- `use_pwm` takes the frequency (Hz) and programs the pin's primary timer
  (24-bit TCC or 8-bit TC, per pin map); `duty()` is runtime-repeated and
  glitch-free on TCC.
- `duty()`/`adc_read()` also carry the `static_assert`, guarding call sites
  that skip the `use_*` call.
- A resource manager gives exclusive ownership of shared resources (PMUX
  slots, SERCOMs, timers, the single ADC). A second claimant is rejected at
  runtime - the pin/peripheral simply stays with its first owner (the `use_*`
  calls return the pin regardless, for chaining).
- `chip.i2c<SERCOM_N>()` returns a persistent I2C master bound to the SERCOM
  whose pads you already muxed with `use_sercom<Peripheral::IIC>()`.

### USB serial / printf

USB CDC (extracted from the Arduino SAMD USB stack, fully interrupt-driven -
no task/poll loop needed). The
`Serial` global and `chip.usb` are an Arduino-compatible `Stream`
(`print`/`println`/`read`/`peek`/`available`) plus a C-style `printf`.

`Serial.printf("Batt %.2fV  P %.1fW\n", v, p)` - simplified C subset:

- specifiers: `%d %i %u %x %X %c %s %f %p %%`
- flags: `+` (force sign), `-` (left-justify), `0` (zero-fill)
- width and `.precision`, e.g. `%5.2f`; `%f` precision = decimals (default 6)
- `h/l/ll/z` modifiers accepted and ignored (everything is 32-bit here)
- unknown specifiers print through literally; truncated formats stop safely
- rounding of `%f` can be off by one last digit at exact .5 boundaries
  (scale-multiply rounding - documented in the header)

Also: global `printf(fmt, ...)` / `eprintf(fmt, ...)` routed to `Serial`.
No heap, no FPU needed; formatting is transport-agnostic (any `Stream`
subclass gets it for free).

### Timing

- `millis()` - 1 ms counter, bumped by the SysTick ISR
- `delay(ms)` - blocking (interrupts still fire; USB stays alive)
- `millis`/`delay` live in `src/time.h`

### Scheduler (cooperative periodic callbacks)

```cpp
// setup()
chip.scheduler.every(200, control);                       // void(*)()
chip.scheduler.every(500, [](){ chip.PA10.toggle(); });   // no-capture lambda
chip.scheduler.every(200, fn, user_data);                 // (void*) + userdata

// loop() - the ONLY integration point
void loop() {
    chip.scheduler.tick();
    ...
}
```

- Callbacks fire **in `loop()` context, never in an ISR** - I2C/ADC/USB/
  `printf` are all safe. Nothing fires while the loop blocks in `delay()`.
- `every()` returns an `EveryHandle`; `handle.cancel()` frees the slot.
  8 slots total; when exhausted `every()` returns an invalid handle.
- `ms == 0` fires once per new millisecond. The first fire is `ms` after
  registration. Missed deadlines fire once and the phase catches up - no
  burst of catch-up calls. Millis arithmetic is wraparound-safe.
- Why not a hardware timer: this keeps user code out of interrupt context
  (no priority/ISR-size concerns, no peripheral stolen from the resource
  manager). If "fire while `delay()` blocks" is ever needed, the extension
  is a dedicated TC whose ISR only sets a pending flag - same API.

### SerialConsole

The unmodified Arduino `SerialConsole` (command-line parsing) lives in the
libs root; like any header-only lib there it is on the include path
automatically - just `#include "SerialConsole.h"`; the `Stream` layer keeps
it compiling against `Serial`. **Do not modify it** - it must stay Arduino-compatible. Note it
references newlib `strcmp`/`strtok`/`strtol`/`strtod`, which pulls those
newlib objects (including the large `strtod` float parser) into every build
that uses the console.

## Build system

Build-related files live in `tools/build/` (CMake project, cross-toolchain
file, linker script, upload script); the root `Makefile` is a one-line
include into it, so `make` works from anywhere. `tools/debug/` holds debug
probe configs (OpenOCD). CMake uses the Unix Makefiles generator; one
`build/` dir is shared by profiles - switching profiles reconfigures and
rebuilds in seconds:

| command        | meaning                        |
|----------------|--------------------------------|
| `make`         | release/fast code, `-O2`       |
| `make debug`   | `-O0 -g3 -gdwarf-4`, for GDB   |
| `make flash`   | `make` + upload via `upload.sh`|
| `make clean`   | remove `build/`                |

Build notes:

- Flags: `-mcpu=cortex-m0plus -mthumb`, `-fno-exceptions -fno-rtti`,
  `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections` (dead code is
  dropped aggressively - `-O0` without these bloats the binary past 120 KB,
  `-O2` lands around 49 KB).
- The chip library compiles to a static lib linked with
  `--whole-archive` (its entry points are `inline`/template - nothing
  "pulls" the members otherwise).
- POST_BUILD: `objcopy` to `build/firmware.bin`; `arm-none-eabi-size`
  printed at the end.

### Flashing

This framework expects a **self-rebooting bootloader in the first 8 KB of
flash** (8-KB SAM-BA / uf2-samd21 / SAM Express style). Everything is keyed
off that assumption: `tools/build/link.ld` starts `.text` at `0x2000`,
`startup.cpp` sets `SCB->VTOR = 0x2000`, and `upload.sh` flashes with
`--offset=0x2000`. A raw 0x0000 app (no bootloader) will not boot through
this toolchain - the vectors/VTOR/offsets would all be wrong.

`tools/build/upload.sh` (run by `make flash`): finds the Trinket by VID/PID among
`/dev/ttyACM*`, 1200-bps DTR touch to reboot into SAM-BA, then `bossac`
write at offset `0x2000` (the first 8 KB hold the bootloader). Requires
`pyserial` (python3), `udevadm`, `fuser`; bossac path is hard-coded in the
script.

### Tests

`src/test/run_tests.sh` - compile-time validation: a battery of tiny
TUs compiled with the cross `g++ -c`; "good" configurations must compile,
"bad" ones (wrong pin for ADC/PWM/SERCOM, swapped SERCOM pins, ...) must
fail **with the library's `static_assert` message**, and the script checks
for that message specifically. Run after touching `src/`:

```sh
bash src/test/run_tests.sh
```

## Creating new projects (`samd-new`)

This repo is meant to be kept in one place on disk and shared by many
applications. `tools/new/samd-new` scaffolds a new project from
`tools/new/template/`:

```sh
tools/new/samd-new my-app            # ./my-app/
tools/new/samd-new my-app ~/projects # ~/projects/my-app/
```

Generated project = `main.cpp` skeleton + `CMakeLists.txt` + `Makefile` +
`.vscode/` + a fresh `openocd.cfg` copied from `tools/debug/`. It builds
immediately (`cd my-app && make` / `make flash` - same targets as the
central repo). `.vscode/` (needs the Cortex-Debug extension) has two
targets in the Run & Debug (F5) dropdown:

| target | action |
|---|---|
| `<name> debug+flash+run` | `make SPEED=debug flash`, then OpenOCD `reset halt` stops at `Reset_Handler` under GDB |
| `<name> release+flash+run` | `make flash`, then same |

The `make`/`make debug`/`make flash` targets remain usable from a plain
terminal for build-only or flash-without-GDB workflows.

The library itself is **never copied** - the project links against the
central repo. Its location, first match wins:

1. `make SAMD21_LIB=/path` (or cmake `-DSAMD21_LIB=/path`)
2. `$SAMD21_LIB` environment variable
3. the path stamped into the generated files at creation time

So moving the library once + `export SAMD21_LIB=/new/path` repoints every
project, and any library change takes effect in all projects on next build
(no resync, no duplication). The central repo and generated projects share
the same CMake module: `tools/build/samd-app.cmake`.

Tip: `ln -s $PWD/tools/new/samd-new ~/.local/bin/samd-new`.

### Extra libraries (libs root)

Additional third-party libs live in a shared **libs root** - default: the
directory *containing* `samd21-lib` (sibling layout), override with
`$SAMD21_LIBS`:

```
~/                       (libs root)
├── samd21-lib/
├── SerialConsole/        headers only
├── SomeLib/              headers + .c/.cpp (no CMakeLists)
└── SomeOtherLib/         ships its own CMakeLists.txt
```

- **Plain libs (no `CMakeLists.txt`) work out of the box**: every
  directory under the libs root is on the include path, so its headers
  are includable flat (`#include "SomeLib.h"`), and its top-level
  `*.c`/`*.cpp` are compiled straight into the firmware. Drop the dir
  into the libs root and it is usable from every project, no wiring.
- **Managed libs** (shipping a `CMakeLists.txt` that defines its own
  targets/flags) need one-time per-project wiring with
  `tools/new/samd-add-lib <name> [project-dir]` - it appends an
  `add_subdirectory()` entry to the project's `libs.cmake`. The stamped
  path can be overridden per lib (`-DSOMEOTHERLIB_LIB=...`) or via
  `$SAMD21_LIBS`; re-adding a lib is a no-op.

Anything in the libs root that is *not* a lib (app projects etc.) has a
`CMakeLists.txt`, so it is never picked up automatically.

## Toolchain & dependencies

- **Arm GNU Toolchain**: Adafruit `arm-none-eabi-gcc 9-2019q4` (fetched by
  Arduino IDE 15), expected at
  `~/.arduino15/packages/adafruit/tools/arm-none-eabi-gcc/9-2019q4/`.
  The CMake cross toolchain file (`tools/build/toolchain.cmake`) sets
  `CMAKE_SYSTEM_NAME Generic` and marks the compilers as working (no host
  link test for bare metal).
- **CMSIS** device headers (`SAMD21E18A`) under `src/libs/cmsis/` -
  register structs and base addresses only.
- **USB CDC-ACM** - header-only (`src/UsbCDC.h`), extracted from the
  Arduino SAMD USB stack; no external USB library.
- **newlib** (from the toolchain) - libc string/`strtod` support, pulled in
  only when something references it (currently: `SerialConsole`).
- No kernel, no RTOS, no heap allocator in the library itself; C++ static
  init goes through `.init_array` handled by the startup code.

## Linker script (`tools/build/link.ld`)

- `FLASH`: origin `0x2000` (after the 8 KB SAM-BA bootloader), 248 KB
- `RAM`: `0x20000000`, 32 KB
- Entry `Reset_Handler`; vector table kept at the front of `.text`
  (`KEEP(*(.vectors))`)
- `.data` is load-flash / run-RAM; `_sidata` points at the `.data` **LMA**
  (not the end of `.text` - other flash sections sit in between), copied by
  startup; `.bss` zeroed
- Minimum stack 2 KB

## Gotchas for agents

- Do **not** modify `SerialConsole/` (Arduino-compatibility constraint).
- A `main.cpp` at the repo root (if present) is scratch test code; the
  library's API is shaped by `src/`, not by the app.
- The `Timer` enumeration in `src/Types.h` is a peripheral-instance type
  (TCC0..TC7), NOT a time-utility name. Periodic-call/timer utilities would
  need a proper design before getting in.
- The SAMD21 has one ADC: `use_adc()` on a second pin re-points the input
  mux (last pin wins); there is no per-channel conversion.
- `use_sercom` is pin-level pad muxing; the peripheral is started by the
  wrapper objects (e.g. `chip.i2c<N>()`).
- GCLK0 = 48 MHz is the clock source assumed by PWM/ADC/SERCOM helpers.
