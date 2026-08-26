// Bad PWM config - PA02 is ADC-only (no timer capability), should fail
// compilation with the use_pwm() static_assert.
#include "Chip.h"

void setup(void) {
    chip.PA02.use_pwm(1000);
}
void loop(void) {}
