// Good I2C config - PA16/PA17 both support SERCOM, should compile successfully
#include "Chip.h"

I2C i2c(chip.PA16, chip.PA17, 400000);

void setup(void) {}
void loop(void) {}
