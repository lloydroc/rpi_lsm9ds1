#include "../config.h"
#include <stdio.h>

#include "lsm9ds1.h"

int
main(int argc, char *argv[])
{
  struct LSM9DS1 dev;

  dev.spidev_ag = "/dev/spidev0.0";
  dev.spidev_m  = "/dev/spidev0.1";
  //dev.spi_clk_hz = 8000000;
  dev.spi_clk_hz = 8000;

  char tx[3] = { WHO_AM_I, 0x00, 0x00 };
  char rx[3];

  tx[0] |= 0x80;

  lsm9ds1_init(&dev);

  lsm9ds1_ag_data(&dev, tx, rx, 2);
  printf("%02x,%02x\n",0b01101000, 0b01101000);
  printf("%02x,%02x\n",rx[0], rx[1]);

  lsm9ds1_deinit(&dev);

  return 0;
}
