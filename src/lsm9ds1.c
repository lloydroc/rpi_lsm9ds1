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
    if(safety-- == 0)
    {
      fprintf(stderr, "Timed out waiting for reset\n");
      return 1;
    }
  }

  return ret;
}

int
lsm9ds1_test(struct LSM9DS1* lsm9ds1)
{
  uint8_t val, val2;
  int ret = 0;
  val = 0;
  lsm9ds1_ag_read(lsm9ds1, WHO_AM_I, &val);
  if(val != WHO_AM_I_VAL)
  {
    fprintf(stderr, "WHO_AM_I register not %d and is %d\n", WHO_AM_I_VAL, val);
    ret++;
  }

  // read a value from the lsm9ds1
  lsm9ds1_ag_read(lsm9ds1, REFERENCE_G, &val2);
  // write arbitrary value
  lsm9ds1_ag_write(lsm9ds1, REFERENCE_G, 0xab);
  // read value back
  lsm9ds1_ag_read(lsm9ds1, REFERENCE_G, &val);
  // chack value
  if(val != 0xab)
  {
    fprintf(stderr, "Unable to write a register and read back the result\n");
    ret++;
  }
  // write original value back
  lsm9ds1_ag_write(lsm9ds1, REFERENCE_G, val2);

  /* would be nice if the datasheet documented what this does
   * and how to read the result
  lsm9ds1_ag_write(lsm9ds1, CTRL_REG10, 0); // disable self-test
  lsm9ds1_ag_write(lsm9ds1, CTRL_REG10, ST_G | ST_XL); // enable self-test
  lsm9ds1_ag_write(lsm9ds1, CTRL_REG10, 0); // disable self-test
  */

  return ret;
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

    if(reg == CTRL_REG1_G || reg == CTRL_REG6_XL)
    {
      // first 3 bits are the odr
      val &= 0b00011111;
      val |= (lsm9ds1->odr << 5);
    }

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
lsm9ds1_configure_ag_interrupt(int gpio, int *fd)
{
  int direction, edge;

  if(gpio_exists())
  {
    fprintf(stderr, "GPIO Directory not found: %s\n", GPIO_PATH);
    return 1;
  }

  if(gpio_permissions_valid())
  {
    fprintf(stderr, "GPIO Permissions not valid on directory %s\n", GPIO_EXPORT_PATH);
    return 1;
  }

  if(gpio_valid(gpio))
  {
    fprintf(stderr, "GPIO %d is not valid\n", gpio);
    return 1;
  }

  if(gpio_export(gpio))
  {
    fprintf(stderr, "Unable to export GPIO %d\n", gpio);
    return 1;
  }

  if(gpio_get_direction(gpio, &direction))
  {
    fprintf(stderr, "Unable to get direction of GPIO %d\n", gpio);
    return 1;
  }

  if(direction == 0)
  {
    if(gpio_set_input(gpio))
    {
      fprintf(stderr, "Unable to set GPIO %d to input\n", gpio);
      return 1;
    }
  }

  if(gpio_get_edge(gpio, &edge))
  {
    fprintf(stderr, "Unable to get edge for GPIO %d\n", gpio);
    return 1;
  }

  if(edge != rising)
  {
    if(gpio_set_edge(gpio, rising))
    {
      fprintf(stderr, "Unable to set rising edge to GPIO %d\n", gpio);
      return 1;
    }
  }

  *fd = gpio_open(gpio, 0);
  if(*fd == -1)
  {
    fprintf(stderr, "Unable to open gpio %d for reading\n", gpio);
    return 1;
  }
  return 0;
}

int
lsm9ds1_unconfigure_ag_interrupt(int gpio, int fd)
{
  gpio_close(fd);
  return gpio_unexport(gpio);
}

int
lsm9ds1_deinit(struct LSM9DS1* lsm9ds1)
{
  int ret;
  ret = spi_destroy(&lsm9ds1->spi_ag);
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
lsm9ds1_ag_print_status(struct LSM9DS1* lsm9ds1)
{
  // TODO
  uint8_t status, val;
  lsm9ds1_ag_read(lsm9ds1, STATUS_REG, &status);
  printf("STATUS_REG: 0x%02x\n", status);

  if(status & BOOT_STATUS)
    printf("Boot running\n");
  if(status & INACT)
    printf("interrupts generated\n");
  if(status & IG_XL)
    printf("XL Interrupt\n");
  if(status & IG_G)
    printf("G  Interrupt\n");
  if(status & TDA)
    printf("T  Data Available\n");
  if(status & GDA)
    printf("G  Data Available\n");
  if(status & XLDA)
    printf("XL Data Available\n");

  lsm9ds1_ag_read(lsm9ds1, INT_GEN_SRC_G, &val);
  printf("INT_GEN_SRC_G: 0x%02x\n", val);

  lsm9ds1_ag_read(lsm9ds1, INT_GEN_SRC_XL, &val);
  printf("INT_GEN_SRC_XL: 0x%02x\n", val);

  lsm9ds1_ag_read(lsm9ds1, INT2_CTRL, &val);
  printf("INT2_CTRL: 0x%02x\n", val);

  lsm9ds1_ag_read(lsm9ds1, INT_GEN_SRC_G, &val);
  printf("INT_GEN_SRC_G: 0x%02x\n", val);

  lsm9ds1_ag_read(lsm9ds1, INT_GEN_SRC_XL, &val);
  printf("INT_GEN_SRC_XL: 0x%02x\n", val);

  /*lsm9ds1_ag_read(lsm9ds1, FIFO_CTRL, &val);
  printf("FIFO_CTRL: 0x%02x\n", val);

  lsm9ds1_ag_read(lsm9ds1, FIFO_SRC, &val);
  printf("FIFO_SRC: 0x%02x\n", val);

  if(val & FTH)
    printf("FIFO level >= threshold\n");
  else
    printf("FIFO level < threshold\n");

  if(val & OVRN)
    printf("FIFO is full\n");
  else
    printf("FIFO not full\n");

  printf("FIFO has %d samples\n", val & FSS);
  */

  lsm9ds1_ag_read_g(lsm9ds1);
  lsm9ds1_ag_read_xl(lsm9ds1);
  lsm9ds1_ag_read(lsm9ds1, STATUS_REG, &status);

  printf("      G: %+5d %+5d %+5d\n", lsm9ds1->g.x, lsm9ds1->g.y, lsm9ds1->g.z);
  printf("     XL: %+5d %+5d %+5d\n", lsm9ds1->xl.x, lsm9ds1->xl.y, lsm9ds1->xl.z);

  printf("STATUS_REG: 0x%02x\n", status);
  return 0;
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

/*
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
*/

int
lsm9ds1_ag_read_xl(struct LSM9DS1* lsm9ds1)
{
  int ret;
  ret = lsm9ds1_ag_read_xyz(lsm9ds1, OUT_X_XL, &lsm9ds1->xl);

  if(ret)
    err_output("lsm9ds1_ag_read_xl");

  return ret;
}

int
lsm9ds1_ag_read_g(struct LSM9DS1* lsm9ds1)
{
  int ret;
  ret = lsm9ds1_ag_read_xyz(lsm9ds1, OUT_X_G, &lsm9ds1->g);

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

void
lsm9ds1_ag_write_terminal(struct LSM9DS1* dev)
{
    printf("\rG(x,y,z)=(%+6d,%+6d,%+6d)", dev->g.x, dev->g.y, dev->g.z);
    printf(" XL(x,y,z)=(%+6d,%+6d,%+6d)", dev->xl.x, dev->xl.y, dev->xl.z);
    fflush(stdout);
}

void
lsm9ds1_ag_write_file(struct LSM9DS1* dev, FILE* file, int binary)
{
  if(binary)
  {
    fwrite(&dev->g,  sizeof(int16_t), 3, file);
    fwrite(&dev->xl, sizeof(int16_t), 3, file);
  }
  else
  {
    const char* format = "%6d,%6d,%6d,%6d,%6d,%6d\n";
    fprintf(file, format, dev->g.x, dev->g.y, dev->g.z, dev->xl.x, dev->xl.y, dev->xl.z);
  }
}
