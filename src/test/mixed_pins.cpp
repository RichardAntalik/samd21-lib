// Mixed valid/invalid I2C config - PA16 is good but PA03 has no SERCOM, should fail compilation
#include "Chip.h"

I2C i2c(chip.PA16, chip.PA03, 400000);

void setup(void) {}
void loop(void) {}
