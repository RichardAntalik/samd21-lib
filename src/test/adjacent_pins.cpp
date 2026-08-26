// Adjacent valid I2C pins on the SAME SERCOM - PA04 (SERCOM0 PAD0) / PA05 (SERCOM0 PAD1)
#include "Chip.h"

I2C i2c(chip.PA04, chip.PA05, 400000);

void setup(void) {}
void loop(void) {}
