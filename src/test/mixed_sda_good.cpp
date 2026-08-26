// Mixed valid/invalid I2C config - PA03 (SCL) has no SERCOM but PA17 (SDA) is fine, should fail with SCL error message
#include "Chip.h"

I2C i2c(chip.PA03, chip.PA17, 400000);

void setup(void) {}
void loop(void) {}
