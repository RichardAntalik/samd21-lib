#pragma once
#include <cstdint>

// ============================================================================
// Scheduler - cooperative periodic callbacks.
//
// Register from setup() (or anywhere on the main thread):
//
//     auto led    = chip.scheduler.every(500, [](){ chip.PA10.toggle(); });
//     auto status = chip.scheduler.every(500, print_status);
//     auto cfg    = chip.scheduler.every(200, fn, user_data);
//
// then advance it once per loop() iteration:
//
//     void loop() { chip.scheduler.tick(); ... }
//
// Callbacks run in *loop* context - not in an ISR. I2C/ADC/USB/printf are
// all safe to use. Nothing fires while the loop is blocked in delay().
//
// Overloads:
//   every(ms, fn)            fn is callable with ()  (no-capture lambda or
//                            void(*)() function pointer)
//   every(ms, fn, user)      fn is callable with (void*) - the user pointer
//                            carries captures/arguments
//
// ms == 0 means "once per new millisecond" (as fast as the loop runs).
// The first fire is `ms` after registration. If the loop was slow and a
// deadline was missed, the callback fires once and the phase catches up -
// no burst of catch-up calls.
//
// The time source is injectable (defaults to millis() from chip/time.h) so
// the tick math can be unit-tested on the host.
// ============================================================================

unsigned long millis(); // defined in chip/time.h (included by Chip.h)

class EveryHandle {
public:
    EveryHandle() noexcept = default;
    bool valid() const noexcept { return slot_ != 0; }
    bool cancel() noexcept;

private:
    friend class Scheduler;
    explicit EveryHandle(uint8_t slot) noexcept : slot_(slot) {}
    uint8_t slot_ = 0;
};

class Scheduler {
public:
    static constexpr uint8_t kMaxTimers = 8;

    using Fn0 = void (*)();
    using Fn1 = void (*)(void*);
    using MillisFn = uint32_t (*)();

    // Time source override (shared by all Scheduler instances).
    static void set_millis(MillisFn fn) noexcept { s_millis = fn; }

    EveryHandle every(uint32_t ms, Fn0 fn) noexcept {
        for (uint8_t i = 0; i < kMaxTimers; i++) {
            Slot& s = s_slots[i];
            if (s.kind) continue;
            s.kind = 1;
            s.f0 = fn;
            s.user = nullptr;
            s.period = ms;
            s.next = s_millis(); // first fire `ms` after registration
            return EveryHandle(i + 1);
        }
        return EveryHandle(); // out of slots
    }

    EveryHandle every(uint32_t ms, Fn1 fn, void* user) noexcept {
        for (uint8_t i = 0; i < kMaxTimers; i++) {
            Slot& s = s_slots[i];
            if (s.kind) continue;
            s.kind = 2;
            s.f1 = fn;
            s.user = user;
            s.period = ms;
            s.next = s_millis();
            return EveryHandle(i + 1);
        }
        return EveryHandle(); // out of slots
    }

    // Advance timers and fire whatever is due. Call once per loop().
    void tick() noexcept {
        const uint32_t now = s_millis();
        for (uint8_t i = 0; i < kMaxTimers; i++) {
            Slot& s = s_slots[i];
            if (!s.kind) continue;

            if (s.period == 0) {
                if (now == s.next) continue; // already fired this ms
                s.next = now;
            } else {
                if (now - s.next < s.period) continue; // not due (wrap-safe)
                // Catch up the phase without bursting.
                while (now - s.next >= s.period) s.next += s.period;
            }
            if (s.kind == 1) s.f0();
            else             s.f1(s.user);
        }
    }

private:
    friend class EveryHandle;

    struct Slot {
        union { Fn0 f0; Fn1 f1; };
        void* user;
        uint32_t period;
        uint32_t next;
        uint8_t kind; // 0 = free, 1 = fn0, 2 = fn1
    };

    static Slot s_slots[kMaxTimers];
    static MillisFn s_millis;
};

inline Scheduler::Slot Scheduler::s_slots[Scheduler::kMaxTimers] = {};
inline Scheduler::MillisFn Scheduler::s_millis = []() -> uint32_t {
    return static_cast<uint32_t>(millis());
};

inline bool EveryHandle::cancel() noexcept {
    if (!valid()) return false;
    Scheduler::Slot& s = Scheduler::s_slots[slot_ - 1];
    if (!s.kind) return false;
    s.kind = 0;
    slot_ = 0;
    return true;
}
