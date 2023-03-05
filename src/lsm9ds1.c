#include "lsm9ds1.h"

/* Interrupt driven for XL and G */
uint8_t LSM9DS1_AG_INIT[][2] =
{
  /* Block Data Update, Register address automatic increment on SPI */
  { CTRL_REG8,      BDU | IF_ADD_INC },
  /* Disable I2C */
  { CTRL_REG9,      I2C_DISABLE      },
  /* Enable G.Z, G.Y, G.Z, XL Latched Interrupt 4D option */
  { CTRL_REG4,      0b00111011       },
  /* Angular Rate Sensor Control Reg 1: Gyroscope ODR 238Hz TODO */
  { CTRL_REG1_G,    0b10000000       },
  /* Angular Rate Sensor Control Reg 3: Enable Highpass Filter */
  { CTRL_REG3_G,    HP_EN            },
  /* Linear Acceleration Sensor Control Reg: Enable Z,Y,X */
  { CTRL_REG5_XL,   0b00111000       },
  /* Linear Acceleration Sensor Control Reg: Odr 238 Hz, BW selected TODO */
  { CTRL_REG6_XL,   0b10000111       },
  /* Latch Gyroscope Interrupt Request */
  { INT_GEN_CFG_G,  LIR_G            },
  /* INT1_A/G pin control register: Interrupt on Gyroscope Data Ready */
  { INT1_CTRL,      INT_DRDY_G       },
  /* Turn off FIFO */
  { FIFO_CTRL,      0b00000000       },
};

size_t LSM9DS1_AG_INIT_SIZE = sizeof(LSM9DS1_AG_INIT);

/*
 * OUCH!!! I spent so much time getting M interrupts to work!
 * I found the documentation wrong for the CTRL_REG3_M on the
 * SPI.
 *
 * Also, the Big Endian must be set or the interrupt won't fire!!!
 * The G and XL data comes in as Little Endian so we need to swap it.
 * It could be a wrong configuration but I've tried many things to
 * no avail.
 */
uint8_t LSM9DS1_M_INIT[][2] =
{
 /* Temperature Compensation, X&Y High Performance, Self Test Disabled */
 { CTRL_REG1_M, 0b11000001 },
 /* +/- 4 Gauss, Normal Mode, No Reset */
 { CTRL_REG2_M, 0b00000000 },
 /* I2C Disable, Single Conversion */
 { CTRL_REG3_M, 0b10000000 },
 /* Z-Axis High Performance Mode, Big Endian */
 { CTRL_REG4_M, 0b00001010 },
 /* Disable Fast Read, Disble Block Update */
 { CTRL_REG5_M, 0b00000000 },
 /* Interrupts threshold low */
 { INT_THS_L_M,   0 },
 /* Interrupts threshold high */
 { INT_THS_H_M,   0 },
 /* Interrupts enabled on INT_M active High for XYZ*/
 { INT_CFG_M,   0b11100101 },
};

size_t LSM9DS1_M_INIT_SIZE = sizeof(LSM9DS1_M_INIT);

static int
lsm9ds1_read(struct SPI* spi, uint8_t reg, uint8_t *data)
{
  int ret;
  uint8_t tx[2], rx[2];
  tx[0] = 0x80 | reg;
  tx[1] = 0;
  ret = spi_transfer(spi, tx, rx, 2);
  *data = rx[1];
  if(ret)
    err_output("lsm9ds1_read");
  return ret;
}

int
lsm9ds1_write(struct SPI* spi, uint8_t reg, uint8_t data)
{
  int ret;
  uint8_t tx[2], rx[2];
  tx[0] = 0x7F & reg;
  tx[1] = data;
  ret = spi_transfer(spi, tx, rx, 2);
  if(ret)
    err_output("lsm9ds1_write");
  return ret;
}

static int
lsm9ds1_read_xyz(struct SPI* spi, uint8_t reg, struct point *xyz)
{
  int ret;
  uint8_t tx[7], rx[7];
  tx[0] = 0x80 | reg;
  for(int i=1; i<7; i++) tx[i] = 0;
  ret = spi_transfer(spi, tx, rx, 7);
  xyz->x = rx[1] + (rx[2] << 8);
  xyz->y = rx[3] + (rx[4] << 8);
  xyz->z = rx[5] + (rx[6] << 8);

  if(ret)
    err_output("lsm9ds1_read_xyz");
  return ret;
}

static int
lsm9ds1_read_m_xyz_be(struct SPI* spi, uint8_t reg, struct point *xyz)
{
  int ret;
  uint8_t tx[7], rx[7];
  tx[0] = 0xC0 | reg;
  for(int i=1; i<7; i++) tx[i] = 0;
  ret = spi_transfer(spi, tx, rx, 7);
  xyz->x = rx[2] + (rx[1] << 8);
  xyz->y = rx[4] + (rx[3] << 8);
  xyz->z = rx[6] + (rx[5] << 8);

  if(ret)
    err_output("lsm9ds1_read_xyz");
  return ret;
}

static int
lsm9ds1_ag_read_status(struct LSM9DS1* lsm9ds1, uint8_t *status)
{
  return lsm9ds1_read(&lsm9ds1->spi_ag, STATUS_REG, status);
}

/*
static int
lsm9ds1_m_read_status(struct LSM9DS1* lsm9ds1, uint8_t *status)
{
  return lsm9ds1_read(&lsm9ds1->spi_m, STATUS_REG_M, status);
}
*/

static int
lsm9ds1_ag_read_xl(struct LSM9DS1* lsm9ds1)
{
  int ret;
  ret = lsm9ds1_read_xyz(&lsm9ds1->spi_ag, OUT_X_XL, &lsm9ds1->xl);

  if(ret)
    err_output("lsm9ds1_ag_read_xl");

  return ret;
}

static int
lsm9ds1_ag_read_g(struct LSM9DS1* lsm9ds1)
{
  int ret;
  ret = lsm9ds1_read_xyz(&lsm9ds1->spi_ag, OUT_X_G, &lsm9ds1->g);

  if(ret)
    err_output("lsm9ds1_ag_read_g");

  return ret;
}


static int
lsm9ds1_m_read(struct LSM9DS1* lsm9ds1)
{
  int ret;
  ret = lsm9ds1_read_m_xyz_be(&lsm9ds1->spi_m, OUT_X_L_M, &lsm9ds1->m);

  if(ret)
    err_output("lsm9ds1_m_read");

  return ret;
}

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

  ret = spi_init(&lsm9ds1->spi_m, lsm9ds1->spidev_m, lsm9ds1->spi_clk_hz, 0, 8, 0);
  if(ret == -1)
  {
    err_output("lsm9ds1_init: opening %s", lsm9ds1->spidev_m);
    return ret;
  }
  return ret;
}

int
lsm9ds1_deinit(struct LSM9DS1* lsm9ds1, struct options *opts)
{
  int ret;
  ret  = lsm9ds1_unconfigure_interrupt(opts->gpio_interrupt_ag, lsm9ds1->fd_int1_ag_pin);
  ret |= lsm9ds1_unconfigure_interrupt(opts->gpio_interrupt_m, lsm9ds1->fd_int1_m_pin);
  ret |= spi_destroy(&lsm9ds1->spi_ag);
  ret |= spi_destroy(&lsm9ds1->spi_m);
  if(opts->data_file != NULL)
    ret |= fclose(opts->data_file);
  if(opts->fd_socket_udp != -1)
    ret |= close(opts->fd_socket_udp);
  return ret;
}

static int
lsm9ds1_ag_reset(struct LSM9DS1* lsm9ds1)
{
  int ret;
  uint8_t status = BOOT;
  int safety = 100000;
  ret  = lsm9ds1_write(&lsm9ds1->spi_ag, CTRL_REG1_G, 0);
  ret |= lsm9ds1_write(&lsm9ds1->spi_ag, CTRL_REG6_XL, 0);
  //ret |= lsm9ds1_write(&lsm9ds1->spi_ag, CTRL_REG8, BOOT | SW_RESET);
  ret |= lsm9ds1_write(&lsm9ds1->spi_ag, CTRL_REG8, BOOT);
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
lsm9ds1_m_reset(struct LSM9DS1* lsm9ds1)
{
  int ret;
  // After we reboot the G sensor, per the datasheet, I don't see a way to
  // poll and determine if the G sensor has rebooted.
  ret = lsm9ds1_write(&lsm9ds1->spi_m, CTRL_REG2_M, 0b00001000);
  return ret;
}

int
lsm9ds1_reset(struct LSM9DS1* lsm9ds1)
{
  int ret;
  ret  = lsm9ds1_ag_reset(lsm9ds1);
  ret |= lsm9ds1_m_reset(lsm9ds1);

  return ret;
}

static int
lsm9ds1_ag_test(struct LSM9DS1* lsm9ds1)
{
  uint8_t val, val2;
  int ret = 0;
  val = 0;
  lsm9ds1_read(&lsm9ds1->spi_ag, WHO_AM_I, &val);
  if(val != WHO_AM_I_VAL)
  {
    fprintf(stderr, "WHO_AM_I register value 0x%02x not 0x%02x\n", val, WHO_AM_I_VAL);
    ret++;
  }

  // read a value from the lsm9ds1
  lsm9ds1_read(&lsm9ds1->spi_ag, REFERENCE_G, &val2);
  // write arbitrary value
  lsm9ds1_write(&lsm9ds1->spi_ag, REFERENCE_G, 0xab);
  // read value back
  lsm9ds1_read(&lsm9ds1->spi_ag, REFERENCE_G, &val);
  // chack value
  if(val != 0xab)
  {
    fprintf(stderr, "Wrote register REFERENCE_G with 0x%02x and read back 0x%02x\n", 0xab, val);
    ret++;
  }

  // write original value back
  lsm9ds1_write(&lsm9ds1->spi_ag, REFERENCE_G, val2);

  /* would be nice if the datasheet documented what this does
   * and how to read the result
  lsm9ds1_write(&lsm9ds1->spi_ag, CTRL_REG10, 0); // disable self-test
  lsm9ds1_write(&lsm9ds1->spi_ag, CTRL_REG10, ST_G | ST_XL); // enable self-test
  lsm9ds1_write(&lsm9ds1->spi_ag, CTRL_REG10, 0); // disable self-test
  */

  return ret;
}

static int
lsm9ds1_m_test(struct LSM9DS1* lsm9ds1)
{
  uint8_t val, val2;
  int ret = 0;
  val = 0;
  lsm9ds1_read(&lsm9ds1->spi_m, WHO_AM_I_M, &val);
  if(val != WHO_AM_I_M_VAL)
  {
    fprintf(stderr, "WHO_AM_I_M register value 0x%02x not 0x%02x\n", val, WHO_AM_I_M_VAL);
    ret++;
  }

  // read a value from the lsm9ds1
  lsm9ds1_read(&lsm9ds1->spi_m, OFFSET_X_REG_L_M, &val2);
  // write arbitrary value
  lsm9ds1_write(&lsm9ds1->spi_m, OFFSET_X_REG_L_M, 0xab);
  // read value back
  lsm9ds1_read(&lsm9ds1->spi_m, OFFSET_X_REG_L_M, &val);
  // chack value
  if(val != 0xab)
  {
    fprintf(stderr, "Wrote register OFFSET_X_REG_L_M with 0x%02x and read back 0x%02x\n", 0xab, val);
    ret++;
  }
  // write original value back
  lsm9ds1_write(&lsm9ds1->spi_m, OFFSET_X_REG_L_M, val2);

  return ret;
}

int
lsm9ds1_test(struct LSM9DS1* lsm9ds1)
{
  int failed;
  failed = lsm9ds1_ag_test(lsm9ds1);
  failed += lsm9ds1_m_test(lsm9ds1);

  return failed;
}

static int
lsm9ds1_configure_ag(struct LSM9DS1* lsm9ds1)
{
  int numregs = LSM9DS1_AG_INIT_SIZE/2;
  uint8_t reg, val, tval;
  int ret = 0;
  for(int i=0;i<numregs;i++)
  {
    reg = LSM9DS1_AG_INIT[i][0];
    val = LSM9DS1_AG_INIT[i][1];

    if(reg == CTRL_REG1_G || reg == CTRL_REG6_XL)
    {
      // first 3 bits are the odr
      val &= 0b00011111;
      val |= (lsm9ds1->odr_ag << 5);
    }

    lsm9ds1_write(&lsm9ds1->spi_ag, reg, val);
    lsm9ds1_read(&lsm9ds1->spi_ag, reg, &tval);

    // read back what we wrote to confirm correct
    if(val != tval)
    {
      fprintf(stderr, "register 0x%x val 0x%x is 0x%x\n", reg, val, tval);
      ret = 1;
    }
  }
  return ret;
}

static int
lsm9ds1_configure_m(struct LSM9DS1* lsm9ds1)
{
  int numregs = LSM9DS1_M_INIT_SIZE/2;
  uint8_t reg, val, tval;
  int ret = 0;
  for(int i=0;i<numregs;i++)
  {
    reg = LSM9DS1_M_INIT[i][0];
    val = LSM9DS1_M_INIT[i][1];

    if(reg == CTRL_REG1_M)
    {
      // and out the odr
      val &= 0b11100011;
      val |= (lsm9ds1->odr_m << 2);
    }

    lsm9ds1_write(&lsm9ds1->spi_m, reg, val);
    lsm9ds1_read(&lsm9ds1->spi_m, reg, &tval);

    // read back what we wrote to confirm correct
    if(val != tval)
    {
      fprintf(stderr, "register 0x%x val 0x%x is 0x%x\n", reg, val, tval);
      ret = 1;
    }
  }
  return ret;
}

int
lsm9ds1_configure(struct LSM9DS1* lsm9ds1)
{
  if(lsm9ds1_configure_ag(lsm9ds1))
    return 1;
  if(lsm9ds1_configure_m(lsm9ds1))
    return 2;
  return 0;
}

int
lsm9ds1_configure_interrupt(int gpio, int *fd)
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

  *fd = gpio_open(gpio, 1);
  if(*fd == -1)
  {
    fprintf(stderr, "Unable to open gpio %d for reading\n", gpio);
    return 1;
  }
  return 0;
}

int
lsm9ds1_unconfigure_interrupt(int gpio, int fd)
{
  gpio_close(fd);
  return gpio_unexport(gpio);
}

void
lsm9ds1_ag_write_terminal(struct LSM9DS1* dev)
{
    printf("\rG(x,y,z)=(%+6d,%+6d,%+6d)", dev->g.x, dev->g.y, dev->g.z);
    printf(" XL(x,y,z)=(%+6d,%+6d,%+6d)", dev->xl.x, dev->xl.y, dev->xl.z);
    printf(" M(x,y,z)=(%+6d,%+6d,%+6d)", dev->m.x, dev->m.y, dev->m.z);
    fflush(stdout);
}

void
lsm9ds1_ag_write_file(struct LSM9DS1* dev, FILE* file, int binary)
{
  if(binary)
  {
    fwrite(&dev->tv, sizeof(struct timeval), 1, file);
    fwrite(&dev->g,  sizeof(int16_t), 3, file);
    fwrite(&dev->xl, sizeof(int16_t), 3, file);
    fwrite(&dev->m, sizeof(int16_t), 3, file);
  }
  else
  {
    const char* format = "%ld.%06ld,%d,%d,%d,%d,%d,%d,%d,%d,%d\n";
    fprintf(file, format, dev->tv.tv_sec, dev->tv.tv_usec, dev->g.x, dev->g.y, dev->g.z, dev->xl.x, dev->xl.y, dev->xl.z, dev->m.x, dev->m.y, dev->m.z);
  }
}

static int
lsm9ds1_ag_write_socket_udp(struct LSM9DS1 *dev, struct options *opts)
{
  ssize_t buffrx;
  struct LSM9DS1_udp_datagram datagram;
  socklen_t servsock_len;
  servsock_len = sizeof(opts->socket_udp_dest);

  datagram.secs = dev->tv.tv_sec;
  datagram.usecs = dev->tv.tv_usec;
  datagram.g_x = dev->g.x;
  datagram.g_y = dev->g.y;
  datagram.g_z = dev->g.z;
  datagram.xl_x = dev->xl.x;
  datagram.xl_y = dev->xl.y;
  datagram.xl_z = dev->xl.z;
  datagram.m_x = dev->m.x;
  datagram.m_y = dev->m.y;
  datagram.m_z = dev->m.z;

  buffrx = sendto(opts->fd_socket_udp, &datagram, sizeof(datagram), 0, (struct sockaddr *)&opts->socket_udp_dest, servsock_len);

  return buffrx == sizeof(struct LSM9DS1_udp_datagram);
}

int
lsm9ds1_ag_poll(struct LSM9DS1 *dev, struct options *opts)
{
  int timeout;
  struct pollfd pfd[2];
  int fd_data_file;
  int ret;
  char buf[8];
  int tty;

  fd_data_file = -1;
  if(opts->data_file != NULL)
    fd_data_file = fileno(opts->data_file);

  tty = isatty(fd_data_file);

  // we will wait forever
  timeout = -1;

  pfd[0].fd = dev->fd_int1_ag_pin;
  pfd[0].events = POLLPRI | POLLERR;
  pfd[1].fd = dev->fd_int1_m_pin;
  pfd[1].events = POLLPRI | POLLERR;

  while(1)
  {
    ret = poll(pfd, 2, timeout);
    if(ret == 0)
    {
      err_output("poll timed out\n");
      break;
    }
    else if (ret < 0)
    {
      err_output("poll");
      break;
    }

    if(pfd[0].revents & pfd[0].events)
    {
      if(lseek(pfd[0].fd, 0, SEEK_SET) == -1)
        err_output("lseek");
      if(read(pfd[0].fd, buf, sizeof(buf)) == -1)
        err_output("read");

      lsm9ds1_ag_read_xl(dev);
      lsm9ds1_ag_read_g(dev);
    }
    else if(pfd[0].revents)
    {
      err_output("unknown event occured xl=%d %d %d\n", pfd[0].revents, POLLPRI, POLLERR);
      continue;
    }

    if(pfd[1].revents & pfd[1].events)
    {
      if(lseek(pfd[1].fd, 0, SEEK_SET) == -1)
        err_output("lseek");
      if(read(pfd[1].fd, buf, sizeof(buf)) == -1)
        err_output("read");

      lsm9ds1_m_read(dev);
      // we will continue. The implication here is that we assume
      // the XL and G sensors are sampling faster so the M data
      // is fused when we receive data for them. This is a crude
      // way of fusing the M data.
      continue;
    }
    else if(pfd[1].revents)
    {
      err_output("unknown event occured m=%d %d %d\n", pfd[1].revents, POLLPRI, POLLERR);
      continue;
    }

    gettimeofday(&dev->tv, NULL);

    if(tty && !opts->silent)
    {
      lsm9ds1_ag_write_terminal(dev);
    }
    if(fd_data_file != -1)
    {
      lsm9ds1_ag_write_file(dev, opts->data_file, opts->binary);
    }
    if(opts->fd_socket_udp != -1)
    {
      lsm9ds1_ag_write_socket_udp(dev, opts);
    }
  }

  close(pfd[0].fd);
  close(pfd[1].fd);

  return 0;
}
