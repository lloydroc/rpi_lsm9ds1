#ifndef LSM9DS1_H
#define LSM9DS1_H

#include <stdint.h>
#include "lsm9ds1_regs.h"
#include "spi.h"
#include "gpio.h"

extern uint8_t LSM9DS1_INIT0[][2];
extern size_t LSM9DS1_INIT0_SIZE;

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
lsm9ds1_configure(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_ag_read(struct LSM9DS1* lsm9ds1, uint8_t reg, uint8_t *data);

int
lsm9ds1_ag_read2(struct LSM9DS1* lsm9ds1, uint8_t reg, uint16_t *data);

int
lsm9ds1_ag_write(struct LSM9DS1* lsm9ds1, uint8_t reg, uint8_t data);

int
lsm9ds1_ag_write2(struct LSM9DS1* lsm9ds1, uint8_t reg, int16_t data);

#endif
