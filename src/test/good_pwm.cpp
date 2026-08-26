// Good PWM config - PA08 (TCC0/WO0, func E) on the 24-bit TCC path and
// PA14 (TC3/WO0, func E) on the 8-bit TC path, with runtime duty() updates.
#include "Chip.h"

void setup(void) {
    chip.PA08.use_pwm(150000);
    chip.PA14.use_pwm(1000);
}
void loop(void) {
    chip.PA08.duty(0.5f);
    chip.PA14.duty(1.0f);
}
