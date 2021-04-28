#ifndef LSM9DS1_H
#define LSM9DS1_H

#include <stdint.h>
#include <poll.h>
#include <sys/time.h>
#include "lsm9ds1_regs.h"
#include "options.h"
#include "spi.h"
#include "gpio.h"

struct point
{
  int16_t x;
  int16_t y;
  int16_t z;
};

struct LSM9DS1
{
  char *spidev_ag;
  char *spidev_m;

  int spi_clk_hz;

  struct SPI spi_ag;
  struct SPI spi_m;

  int odr_ag;
  int odr_m;

  int int1_ag_pin;
  int fd_int1_ag_pin;

  int int1_m_pin;
  int fd_int1_m_pin;

  struct point g;
  struct point xl;
  struct point m;

  struct timeval tv;
};

struct LSM9DS1_udp_datagram
{
  uint32_t secs;
  uint32_t usecs;
  int16_t g_x, g_y, g_z;
  int16_t xl_x, xl_y, xl_z;
  int16_t m_x, m_y, m_z;
};

int
lsm9ds1_init(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_deinit(struct LSM9DS1* lsm9ds1, struct options *opts);

int
lsm9ds1_reset(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_test(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_configure(struct LSM9DS1* lsm9ds1);

int
lsm9ds1_configure_interrupt(int gpio, int *fd);

int
lsm9ds1_unconfigure_interrupt(int gpio, int fd);

void
lsm9ds1_ag_write_terminal(struct LSM9DS1* dev);

void
lsm9ds1_ag_write_file(struct LSM9DS1* dev, FILE* file, int binary);

int
lsm9ds1_ag_poll(struct LSM9DS1 *dev, struct options *opts);

#endif
