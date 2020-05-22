#include "lsm9ds1.h"

uint8_t LSM9DS1_INIT0[][2] =
{
  { CTRL_REG6_XL, 0b00100011 },
  { CTRL_REG1_G , 0b00100011 },
  { INT1_CTRL,    INT_FTH    }, // FIFO Threshold Interrupt Enabled on INT1_A/g pin
  { CTRL_REG9,    I2C_DISABLE | FIFO_EN | STOP_ON_FTH },
//  { FIFO_CTRL,    0b00000000 }, // reset FIFO by turning off
  { FIFO_CTRL,    0b001 | 30 }, // FIFO Mode with Threshold samples <= 32
};

size_t LSM9DS1_INIT0_SIZE = sizeof(LSM9DS1_INIT0);

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
lsm9ds1_configure(struct LSM9DS1* lsm9ds1)
{
  int numregs = LSM9DS1_INIT0_SIZE/2;
  uint8_t reg, val;
  for(int i=0;i<numregs;i++)
  {
    reg = LSM9DS1_INIT0[i][0];
    val = LSM9DS1_INIT0[i][1];
    lsm9ds1_ag_write(lsm9ds1, reg, val);
  }
  return 0;
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
lsm9ds1_ag_read(struct LSM9DS1* lsm9ds1, uint8_t reg, uint8_t *data)
{
  int ret;
  uint8_t tx[2], rx[2];
  tx[0] = 0x80 | reg;
  ret = spi_transfer(&lsm9ds1->spi_ag, tx, rx, 2);
  *data = rx[1];
  if(ret)
    err_output("lsm9ds1_ag_data");
  return ret;
}

int
lsm9ds1_ag_read2(struct LSM9DS1* lsm9ds1, uint8_t reg, uint16_t *data)
{
  int ret;
  uint8_t tx[3], rx[3];
  tx[0] = 0x80 | reg;
  ret = spi_transfer(&lsm9ds1->spi_ag, tx, rx, 3);
  *data = rx[1];
  if(ret)
    err_output("lsm9ds1_ag_data");
  return ret;
}

int
lsm9ds1_ag_write(struct LSM9DS1* lsm9ds1, uint8_t reg, uint8_t data)
{
  int ret;
  uint8_t tx[2], rx[2];
  tx[0] = 0x7F & reg;
  tx[1] = data;
  ret = spi_transfer(&lsm9ds1->spi_ag, tx, rx, 2);
  if(ret)
    err_output("lsm9ds1_ag_data");
  return ret;
}

int
lsm9ds1_ag_write2(struct LSM9DS1* lsm9ds1, uint8_t reg, int16_t data)
{
  int ret;
  uint8_t tx[3], rx[2];
  tx[0] = 0x7F & reg;
  tx[1] = (int8_t) data;
  tx[2] = (int8_t) (data << 8);
  ret = spi_transfer(&lsm9ds1->spi_ag, tx, rx, 2);
  if(ret)
    err_output("lsm9ds1_ag_data");
  return ret;
}

