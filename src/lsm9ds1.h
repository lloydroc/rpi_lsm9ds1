#ifndef LSM9DS1_H
#define LSM9DS1_H

#include "lsm9ds1_regs.h"
#include "spi.h"
#include "gpio.h"

struct LSM9DS1
{
  char *spidev_ag;
  char *spidev_m;

  int spi_clk_hz;

  struct SPI spi_ag;
  struct SPI spi_m;

  int int1_ag_pin;
  int int2_ag_pin;

  int fd_int1_ag_pin;
  int fd_int2_ag_pin;
};

int
lsm9ds1_init(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_deinit(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_ag_data(struct LSM9DS1* lsm9ds1, char tx[], char rx[], int len);

#endif
