#include "../config.h"
#include <stdio.h>

#include "lsm9ds1.h"

int
main(int argc, char *argv[])
{
  struct LSM9DS1 dev;

  dev.spidev_ag = "/dev/spidev0.0";
  dev.spidev_m  = "/dev/spidev0.1";
  dev.spi_clk_hz = 8000000;

  char tx[1] = { WHO_AM_I };
  //char tx[1] = { 0x0f };
  char rx[1];

  lsm9ds1_init(&dev);

  lsm9ds1_ag_data(&dev, tx, 1, rx, 1);

  lsm9ds1_deinit(&dev);

  return 0;
}
