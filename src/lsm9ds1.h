#ifndef LSM9DS1_H
#define LSM9DS1_H

#include <stdint.h>
#include "lsm9ds1_regs.h"
#include "options.h"
#include "spi.h"
#include "gpio.h"

extern uint8_t LSM9DS1_INIT0[][2];
extern size_t LSM9DS1_INIT0_SIZE;

struct point
{
  int16_t x;
  int16_t y;
  int16_t z;
};

struct LSM9DS1
{
  char *spidev_ag;

  int spi_clk_hz;

  struct SPI spi_ag;

  int odr;

  int int1_ag_pin;
  int int2_ag_pin;

  int fd_int1_ag_pin;
  int fd_int2_ag_pin;

  struct point g;
  struct point xl;
};

int
lsm9ds1_init(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_deinit(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_reset(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_test(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_configure(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_configure_ag_interrupt(int gpio, int *fd);

int
lsm9ds1_unconfigure_ag_interrupt(int gpio, int fd);

int
lsm9ds1_ag_read(struct LSM9DS1* lsm9ds1, uint8_t reg, uint8_t *data);

int
lsm9ds1_ag_read2(struct LSM9DS1* lsm9ds1, uint8_t reg, int16_t *data);

int
lsm9ds1_ag_read_status(struct LSM9DS1* lsm9ds1, uint8_t *status);

int
lsm9ds1_ag_print_status(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_ag_read_xyz(struct LSM9DS1* lsm9ds1, uint8_t reg, struct point *xyz);

int
lsm9ds1_ag_read_xl(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_ag_read_g(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_ag_write_int_g_thresh(struct LSM9DS1* lsm9ds1, struct point *xyz, int dcrm, int wait, uint8_t dur);

int
lsm9ds1_ag_write(struct LSM9DS1* lsm9ds1, uint8_t reg, uint8_t data);

int
lsm9ds1_ag_write2(struct LSM9DS1* lsm9ds1, uint8_t reg, int16_t data);

void
lsm9ds1_ag_write_terminal(struct LSM9DS1* dev);

void
lsm9ds1_ag_write_file(struct LSM9DS1* dev, FILE* file, int binary);

#endif
