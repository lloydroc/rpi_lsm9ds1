#include "lsm9ds1.h"

/* Interrupt driven for XL and G */
uint8_t LSM9DS1_INIT0[][2] =
{
  { CTRL_REG8,      BDU | IF_ADD_INC },
  { CTRL_REG9,      I2C_DISABLE      },
  { CTRL_REG4,      0b00111011       },
  { CTRL_REG1_G,    0b10000000       },
  { CTRL_REG3_G,    HP_EN            },
  { CTRL_REG5_XL,   0b00111000       },
  { CTRL_REG6_XL,   0b10000111       },
  { INT_GEN_CFG_G,  LIR_G            },
  { INT1_CTRL,      INT_DRDY_G       },
  { FIFO_CTRL,      0b00000000       },
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
lsm9ds1_deinit(struct LSM9DS1* lsm9ds1, struct options *opts)
{
  int ret;
  ret  = lsm9ds1_unconfigure_ag_interrupt(opts->gpio_interrupt_ag, lsm9ds1->fd_int1_ag_pin);
  ret |= spi_destroy(&lsm9ds1->spi_ag);
  if(opts->data_file != NULL)
    ret |= fclose(opts->data_file);
  if(opts->fd_socket_udp != -1)
    ret |= close(opts->fd_socket_udp);
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
    fprintf(stderr, "WHO_AM_I register value 0x%02x not 0x%02x\n", val, WHO_AM_I_VAL);
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
    fprintf(stderr, "Wrote register REFERENCE_G with 0x%02x and read back 0x%02x\n", 0xab, val);
    ret++;
  }
  // write original value back
  lsm9ds1_ag_write(lsm9ds1, REFERENCE_G, val2);

  lsm9ds1_ag_read(lsm9ds1, INT_GEN_CFG_G, &val);
  if(val != LIR_G)
  {
    fprintf(stderr, "Interrupts not configured. Has the LSM9DS1 been configured?\n");
    ret++;
  }

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
lsm9ds1_ag_read(struct LSM9DS1* lsm9ds1, uint8_t reg, uint8_t *data)
{
  int ret;
  uint8_t tx[2], rx[2];
  tx[0] = 0x80 | reg;
  tx[1] = 0;
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
  tx[1] = 0; tx[2] = 0;
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
  for(int i=1; i<7; i++) tx[i] = 0;
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
    fwrite(&dev->tv, sizeof(struct timeval), 1, file);
    fwrite(&dev->g,  sizeof(int16_t), 3, file);
    fwrite(&dev->xl, sizeof(int16_t), 3, file);
  }
  else
  {
    const char* format = "%ld.%06ld,%d,%d,%d,%d,%d,%d\n";
    fprintf(file, format, dev->tv.tv_sec, dev->tv.tv_usec, dev->g.x, dev->g.y, dev->g.z, dev->xl.x, dev->xl.y, dev->xl.z);
  }
}

static
int
lsm9ds1_ag_write_socket_udp(struct LSM9DS1 *dev, struct options *opts)
{
  ssize_t buffrx;
  struct LSM9DS1_udp_datagram datagram;
  socklen_t servsock_len;
  servsock_len = sizeof(opts->socket_udp_dest);

  datagram.secs = htonl(dev->tv.tv_sec);
  datagram.usecs = htonl(dev->tv.tv_usec);
  datagram.g_x = htons(dev->g.x);
  datagram.g_y = htons(dev->g.y);
  datagram.g_z = htons(dev->g.z);
  datagram.xl_x = htons(dev->xl.x);
  datagram.xl_y = htons(dev->xl.y);
  datagram.xl_z = htons(dev->xl.z);

  buffrx = sendto(opts->fd_socket_udp, &datagram, sizeof(datagram), 0, (struct sockaddr *)&opts->socket_udp_dest, servsock_len);

  return buffrx == sizeof(struct LSM9DS1_udp_datagram);
}

int
lsm9ds1_ag_poll(struct LSM9DS1 *dev, struct options *opts)
{
  int timeout;
  struct pollfd pfd;
  int fd, fd_data_file;
  int ret;
  char buf[8];

  fd_data_file = -1;
  if(opts->data_file != NULL)
    fd_data_file = fileno(opts->data_file);

  // we will wait forever
  timeout = -1;

  fd = dev->fd_int1_ag_pin;

  pfd.fd = fd;
  pfd.events = POLLPRI;

  while(1)
  {
    ret = poll(&pfd, 1, timeout);
    if(ret == 0)
    {
      fprintf(stderr, "poll timed out\n");
      break;
    }
    else if (ret < 0)
    {
      err_output("poll");
      break;
    }

    lseek(fd, 0, SEEK_SET);
    read(fd, buf, sizeof(buf));

    pfd.fd = fd;

    lsm9ds1_ag_read_xl(dev);
    lsm9ds1_ag_read_g(dev);

    gettimeofday(&dev->tv, NULL);

    if(fd_data_file != -1 && isatty(fd_data_file))
    {
      lsm9ds1_ag_write_terminal(dev);
    }
    else if(fd_data_file != -1)
    {
      lsm9ds1_ag_write_file(dev, opts->data_file, opts->binary);
    }
    if(opts->fd_socket_udp != -1)
    {
      lsm9ds1_ag_write_socket_udp(dev, opts);
    }
  }

  close(fd);

  return 0;
}
