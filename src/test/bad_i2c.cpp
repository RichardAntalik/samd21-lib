// Bad I2C config - PA03 does NOT support SERCOM (no UART/SPI/I2C), should fail compilation
#include "Chip.h"

I2C i2c(chip.PA03, chip.PA05, 400000);

void setup(void) {}
void loop(void) {}
