// Bad ADC read - PA14 does NOT support ADC, reading it should fail compilation
#include "Chip.h"

void setup(void) {}
void loop(void) {
    chip.PA14.adc_read();
}
