// Chained pin API - use_*() must return the pin so member calls can be
// strung together: chip.PA08.use_pwm(150470).duty(0.0f)
#include "Chip.h"

void setup() {
    chip.init();

    chip.PA07.use_out().set(1);
    chip.PA10.use_out().toggle();
    chip.PA08.use_pwm(150470).duty(0.0f);
    chip.PA08.use_pwm(150470).duty(0.5f).duty(0.25f);
    chip.PA05.use_adc().dac_write(0);
    chip.PA03.use_in();

    volatile uint16_t v = chip.PA05.adc_read();
    (void)v;
}
