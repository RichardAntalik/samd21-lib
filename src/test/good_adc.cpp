// Good ADC config - PA02 (AIN0) and PA05 (AIN5) both support ADC. The single
// shared ADC peripheral is brought up once and re-pointed at each pin's
// channel; the last use_adc() is the channel adc_read() measures.
#include "Chip.h"

void setup(void) {
    chip.PA02.use_adc();
    chip.PA04.use_adc();
    chip.PA05.use_adc();
}
void loop(void) {
    chip.PA05.adc_read();
}
