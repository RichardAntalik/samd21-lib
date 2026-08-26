// SDA/SCL on DIFFERENT SERCOMs - PA17 is SERCOM1 but PA04 is SERCOM0, should fail
#include "Chip.h"

I2C i2c(chip.PA17, chip.PA04, 400000);

void setup(void) {}
void loop(void) {}
