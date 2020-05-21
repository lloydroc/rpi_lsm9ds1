#include "lsm9ds1.h"

int
lsm9ds1_init(struct LSM9DS1* lsm9ds1)
{
  int ret;

  ret = spi_init(&lsm9ds1->spi_ag, lsm9ds1->spidev_ag, lsm9ds1->spi_clk_hz, 0, 8, 0);
  if(ret == -1)
  {
    err_output("lsm9ds1_init: opening %s", lsm9ds1->spidev_ag);
    return ret;
  }

  ret = spi_init(&lsm9ds1->spi_ag, lsm9ds1->spidev_m, lsm9ds1->spi_clk_hz, 0, 8, 0);
  if(ret == -1)
  {
    err_output("lsm9ds1_init: opening %s", lsm9ds1->spidev_m);
    return ret;
  }

  return ret;
}

int
lsm9ds1_deinit(struct LSM9DS1* lsm9ds1)
{
  int ret;
  ret = spi_destroy(&lsm9ds1->spi_ag);
  ret |= spi_destroy(&lsm9ds1->spi_m);
  return ret;
}

int
lsm9ds1_ag_data(struct LSM9DS1* lsm9ds1, char tx[], char rx[], int len)
{
  int ret;
  ret = spi_transfer(&lsm9ds1->spi_ag, tx, rx, len);
  if(ret)
    err_output("lsm9ds1_ag_data");
  return ret;
}
