// Bad PWM config - PA03 has no timer, so duty() must fail compilation
// with its static_assert too (guards call sites that skip use_pwm()).
#include "Chip.h"

void setup(void) {}
void loop(void) {
    chip.PA03.duty(0.5f);
}
