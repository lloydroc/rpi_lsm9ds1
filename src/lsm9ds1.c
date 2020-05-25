#include "lsm9ds1.h"

/* Interrupt driven for XL and G */
uint8_t LSM9DS1_INIT0[][2] =
{
//  { CTRL_REG8,      BDU | H_LACTIVE | IF_ADD_INC },
  { CTRL_REG8,      BDU | IF_ADD_INC },
  { CTRL_REG9,      I2C_DISABLE },
  { CTRL_REG4,      0b00111011 },
  { CTRL_REG1_G,    0b10000001 }, // G Angular rate sensor control
  { CTRL_REG5_XL,   0b00111000 },
  { CTRL_REG6_XL,   0b10000111 }, // XL
//  { INT_GEN_CFG_XL, 0b01111111 }, // 6 Direction detection, ALL Interrupts
//  { INT_GEN_CFG_G,  ZHIE_G },
  { INT_GEN_CFG_G,  LIR_G },
//  { INT1_CTRL,      INT1_IG_G | INT1_IG_XL | INT_DRDY_G | INT_DRDY_XL },
  { INT1_CTRL,        INT_DRDY_G },
//  { INT1_CTRL,        INT1_IG_G },
//  { INT1_CTRL,      INT1_IG_G |  INT_DRDY_G },
//  { INT2_CTRL,      INT2_INACT }, // Interruprt on INT2_A/G Pin when G Data Ready
  { FIFO_CTRL,      0b00000000 }, // FIFO OFF
};

/* (unused) FIFO Mode - untested and hard to work with mixed data */
uint8_t LSM9DS1_INIT1[][2] =
{
  { ACT_THS,        SLEEP_ON_INACT_EN}, // gyro sleep not power down
  { CTRL_REG8,      IF_ADD_INC },
  { CTRL_REG9,      SLEEP_G | I2C_DISABLE | FIFO_EN | STOP_ON_FTH },
  { CTRL_REG4,      0b00111000 },
  { CTRL_REG5_XL,   0b00111000 },
  { CTRL_REG6_XL,   0b00100011 },
  { CTRL_REG1_G ,   0b00100011 },
  { INT_GEN_CFG_XL, 0b01000000 }, // 6 Direction detection
  { INT1_CTRL,      INT_FTH    }, // FIFO Threshold Interrupt Enabled on INT1_A/g pin
  { FIFO_CTRL,      0b00000000 }, // reset FIFO by turning off
  { FIFO_CTRL,      0b11000000 | 30 }, // Continuous Mode with Threshold samples <= 32
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

  lsm9ds1->bias_xl.x = 0;
  lsm9ds1->bias_xl.y = 0;
  lsm9ds1->bias_xl.z = 0;
  lsm9ds1->bias_g.x = 0;
  lsm9ds1->bias_g.y = 0;
  lsm9ds1->bias_g.z = 0;

  return ret;
}

int
lsm9ds1_reset(struct LSM9DS1* lsm9ds1)
{
  int ret;
  uint8_t status = BOOT;
  int safety = 100000;
  ret  = lsm9ds1_ag_write(lsm9ds1, CTRL_REG1_G, 0);
  ret |= lsm9ds1_ag_write(lsm9ds1, CTRL_REG6_XL, 0);
  //ret |= lsm9ds1_ag_write(lsm9ds1, CTRL_REG8, BOOT | SW_RESET);
  ret |= lsm9ds1_ag_write(lsm9ds1, CTRL_REG8, BOOT);
  while(status & BOOT)
  {
    lsm9ds1_ag_read_status(lsm9ds1, &status);
    printf("reset");
    if(safety-- == 0)
      return 1;
  }
  return ret;
}

int
lsm9ds1_test(struct LSM9DS1* lsm9ds1)
{
    lsm9ds1_ag_write(lsm9ds1, CTRL_REG10, 0); // disable self-test
    lsm9ds1_ag_write(lsm9ds1, CTRL_REG10, ST_G | ST_XL); // enable self-test
    lsm9ds1_ag_write(lsm9ds1, CTRL_REG10, 0); // disable self-test

    return 0;
}

int
lsm9ds1_configure(struct LSM9DS1* lsm9ds1)
{
  int numregs = LSM9DS1_INIT0_SIZE/2;
  uint8_t reg, val, tval;
  int ret = 0;
  for(int i=0;i<numregs;i++)
  {
    reg = LSM9DS1_INIT0[i][0];
    val = LSM9DS1_INIT0[i][1];
    lsm9ds1_ag_write(lsm9ds1, reg, val);
    lsm9ds1_ag_read(lsm9ds1, reg, &tval);

    // read back what we wrote to confirm correct
    if(val != tval)
    {
      fprintf(stderr, "register %d val %d is %d\n", reg, val, tval);
      ret = 1;
    }
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
lsm9ds1_ag_read2(struct LSM9DS1* lsm9ds1, uint8_t reg, int16_t *data)
{
  int ret;
  uint8_t tx[3], rx[3];
  tx[0] = 0x80 | reg;
  ret = spi_transfer(&lsm9ds1->spi_ag, tx, rx, 3);
  *data = rx[1] + (rx[2] << 8);
  if(ret)
    err_output("lsm9ds1_ag_data");
  return ret;
}

int
lsm9ds1_ag_read_status(struct LSM9DS1* lsm9ds1, uint8_t *status)
{
  return lsm9ds1_ag_read(lsm9ds1, STATUS_REG, status);
}

int
lsm9ds1_ag_read_xyz(struct LSM9DS1* lsm9ds1, uint8_t reg, struct point *xyz)
{
  int ret;
  uint8_t tx[7], rx[7];
  tx[0] = 0x80 | reg;
  ret = spi_transfer(&lsm9ds1->spi_ag, tx, rx, 7);
  xyz->x = rx[1] + (rx[2] << 8);
  xyz->y = rx[3] + (rx[4] << 8);
  xyz->z = rx[5] + (rx[6] << 8);

  if(ret)
    err_output("lsm9ds1_ag_read_xyz");
  return ret;
}

static int
lsm9ds1_ag_read_average_xyz(struct LSM9DS1* lsm9ds1, uint8_t reg, int N, uint8_t mask, struct point *xyz)
{
  int ret;
  uint8_t status = 0;
  int32_t acc_x, acc_y, acc_z;

  acc_x = 0;
  acc_y = 0;
  acc_z = 0;

  for(int i=0;i<N;i++)
  {
    status = 0;
    while((status & mask) == 0)
    {
      ret = lsm9ds1_ag_read_status(lsm9ds1, &status);
    }
    ret = lsm9ds1_ag_read_xyz(lsm9ds1, reg, xyz);
    printf("%3d %+d %+d %+d\n", i, xyz->x, xyz->y, xyz->z);
    acc_x += xyz->x;
    acc_y += xyz->y;
    acc_z += xyz->z;
  }

  acc_x /= N;
  acc_y /= N;
  acc_z /= N;

  xyz->x = acc_x;
  xyz->y = acc_y;
  xyz->z = acc_z;

  if(ret)
    err_output("lsm9ds1_axl_read_xl");

  return ret;
}

int
lsm9ds1_ag_read_xl_bias(struct LSM9DS1* lsm9ds1)
{
  int ret;

  ret = lsm9ds1_ag_read_average_xyz(lsm9ds1, OUT_X_XL, 100, XLDA, &lsm9ds1->bias_xl);

  if(ret)
    err_output("lsm9ds1_axl_read_xl");

  return ret;
}

int
lsm9ds1_ag_read_g_bias(struct LSM9DS1* lsm9ds1)
{
  int ret;

  ret = lsm9ds1_ag_read_average_xyz(lsm9ds1, OUT_X_G, 100, GDA, &lsm9ds1->bias_g);

  if(ret)
    err_output("lsm9ds1_ag_read_g");

  return ret;
}

int
lsm9ds1_ag_read_xl(struct LSM9DS1* lsm9ds1)
{
  int ret;
  struct point xyz;
  ret = lsm9ds1_ag_read_xyz(lsm9ds1, OUT_X_XL, &xyz);

  lsm9ds1->xl.x = xyz.x - lsm9ds1->bias_xl.x;
  lsm9ds1->xl.y = xyz.y - lsm9ds1->bias_xl.y;
  lsm9ds1->xl.z = xyz.z - lsm9ds1->bias_xl.z;

  if(ret)
    err_output("lsm9ds1_ag_read_xl");

  return ret;
}

int
lsm9ds1_ag_read_g(struct LSM9DS1* lsm9ds1)
{
  int ret;
  struct point xyz;
  ret = lsm9ds1_ag_read_xyz(lsm9ds1, OUT_X_G, &xyz);

  lsm9ds1->g.x = xyz.x - lsm9ds1->bias_g.x;
  lsm9ds1->g.y = xyz.y - lsm9ds1->bias_g.y;
  lsm9ds1->g.z = xyz.z - lsm9ds1->bias_g.z;

  if(ret)
    err_output("lsm9ds1_ag_read_g");

  return ret;
}

int
lsm9ds1_ag_write_int_g_thresh(struct LSM9DS1* lsm9ds1, struct point *xyz, int dcrm, int wait, uint8_t dur)
{
  int ret;

  if(dcrm)
    xyz->x |= DCRM_G;
  else
    xyz->x &= 0x7fff;

  ret  = lsm9ds1_ag_write2(lsm9ds1, INT_GEN_THS_X_G, xyz->x);
  ret |= lsm9ds1_ag_write2(lsm9ds1, INT_GEN_THS_Y_G, xyz->y & 0x7fff);
  ret |= lsm9ds1_ag_write2(lsm9ds1, INT_GEN_THS_Z_G, xyz->z & 0x7fff);

  if(wait)
    dur |= WAIT_G;
  ret |= lsm9ds1_ag_write(lsm9ds1, INT_GEN_DUR_G, dur);

  ret |= lsm9ds1_ag_read2(lsm9ds1, INT_GEN_THS_X_G, &xyz->x);
  ret |= lsm9ds1_ag_read2(lsm9ds1, INT_GEN_THS_Y_G, &xyz->y);
  ret |= lsm9ds1_ag_read2(lsm9ds1, INT_GEN_THS_Z_G, &xyz->z);

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
  uint8_t tx[3], rx[3];
  tx[0] = 0x7F & reg;
  tx[1] = (int8_t) (data >> 8);
  tx[2] = (int8_t) data;
  ret = spi_transfer(&lsm9ds1->spi_ag, tx, rx, 3);
  if(ret)
    err_output("lsm9ds1_ag_data");
  return ret;
}
