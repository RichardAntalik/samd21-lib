// Bad ADC config - PA14 does NOT support ADC (no analog input), should fail compilation
#include "Chip.h"

void setup(void) {
    chip.PA14.use_adc();
}
void loop(void) {}
