#include "../config.h"
#include <stdio.h>

#include <stdint.h>
#include <poll.h>
#include <sys/time.h>
#include "error.h"
#include "options.h"
#include "lsm9ds1.h"

/*
int
configure_interrupt(int gpio)
{
  if(gpio_exists())
    _exit(77);

  gpio_permissions_valid();
  gpio_unexport(gpio);

  if(gpio_valid(gpio))
  {
    return EXIT_FAILURE;
  }

  if(gpio_export(gpio))
  {
    return EXIT_FAILURE;
  }

  if(gpio_set_edge(gpio))
  {
    return EXIT_FAILURE;
  }

  return gpio_open_edge(gpio);
}

int
unconfigure_interrupt(int gpio, int fd)
{
  gpio_close(fd);
  return gpio_unexport(gpio);
}
*/

int
get_data(struct LSM9DS1 *dev)
{
  int timeout;
  struct pollfd pfd;
  int fd;
  int ret;
  char buf[8];

  timeout = 1000;

  if((fd = open("/sys/class/gpio/gpio13/value", O_RDONLY)) < 0)
  {
    fprintf(stderr, "Failed to open gpio\n");
    return 1;
  }

  pfd.fd = fd;

  pfd.events = POLLPRI;

  /*
  lseek(fd, 0, SEEK_SET);
  read(fd, buf, sizeof(buf));
  */

  for(int i=0;i<3;i++)
  {
    ret = poll(&pfd, 1, timeout);
    if(ret == 0)
    {
      fprintf(stderr, "poll timed out\n");
      continue;
    }
    else if (ret < 0)
    {
      err_output("poll");
      continue;
    }

    lseek(fd, 0, SEEK_SET);
    read(fd, buf, sizeof(buf));

    pfd.fd = fd;

    lsm9ds1_ag_read_g(dev);
    lsm9ds1_ag_read_xl(dev);
  }

  close(fd);

  return 1;
}

int
main(int argc, char *argv[])
{
  struct options opts;
  struct LSM9DS1 dev;
  uint8_t status = 0;
  uint8_t val = 0;
  int ret;

  options_init(&opts);
  ret = options_parse(&opts, argc, argv);
  if(ret)
  {
  	usage();
      return EXIT_FAILURE;
  }

  dev.spidev_ag = "/dev/spidev0.0";
  dev.spidev_m  = "/dev/spidev0.1";
  dev.spi_clk_hz = opts.spi_clk_hz;

  lsm9ds1_init(&dev);
  
  if(opts.help)
  {
    usage();
    return EXIT_SUCCESS;
  }
  else if(opts.reset)
  {
    lsm9ds1_reset(&dev);
  }
  else if(opts.configure)
  {
    printf("configuring LSM9DS1\n");
    lsm9ds1_configure(&dev);
    lsm9ds1_test(&dev);
  }
  else if(opts.calibrate)
  {
    printf("setting LSM9DS1 bias\n");
    lsm9ds1_ag_read_g_bias(&dev);
    lsm9ds1_ag_read_xl_bias(&dev);
  }
  else if(opts.interrupt_thresh_g)
  {
    printf("setting LSM9DS1 G INT Thresholds\n");
    struct point thresh;
    thresh.x = 0x0000;
    thresh.y = 0x3030;
    thresh.z = 0x0000;
    lsm9ds1_ag_write_int_g_thresh(&dev, &thresh, 0, 0, 0);
    printf("G INT Thresh: 0x%4x 0x%4x 0x%4x\n", thresh.x, thresh.y, thresh.z);
  }

  lsm9ds1_ag_read(&dev, WHO_AM_I, &val);
  printf("WHO_AM_I: 0x%02x\n", val);

  lsm9ds1_ag_read(&dev, STATUS_REG, &status);
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

  lsm9ds1_ag_read(&dev, INT_GEN_SRC_G, &val);
  printf("INT_GEN_SRC_G: 0x%02x\n", val);

  lsm9ds1_ag_read(&dev, INT_GEN_SRC_XL, &val);
  printf("INT_GEN_SRC_XL: 0x%02x\n", val);

  lsm9ds1_ag_read(&dev, INT2_CTRL, &val);
  printf("INT2_CTRL: 0x%02x\n", val);

  lsm9ds1_ag_read(&dev, INT_GEN_SRC_G, &val);
  printf("INT_GEN_SRC_G: 0x%02x\n", val);

  lsm9ds1_ag_read(&dev, INT_GEN_SRC_XL, &val);
  printf("INT_GEN_SRC_XL: 0x%02x\n", val);

  /*lsm9ds1_ag_read(&dev, FIFO_CTRL, &val);
  printf("FIFO_CTRL: 0x%02x\n", val);

  lsm9ds1_ag_read(&dev, FIFO_SRC, &val);
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

  printf("XL_BIAS: %+5d %+5d %+5d\n", dev.bias_xl.x, dev.bias_xl.y, dev.bias_xl.z);
  printf(" G_BIAS: %+5d %+5d %+5d\n", dev.bias_g.x, dev.bias_g.y, dev.bias_g.z);

  lsm9ds1_ag_read_g(&dev);
  lsm9ds1_ag_read_xl(&dev);
  lsm9ds1_ag_read(&dev, STATUS_REG, &status);

  printf("      G: %+5d %+5d %+5d\n", dev.g.x, dev.g.y, dev.g.z);
  printf("     XL: %+5d %+5d %+5d\n", dev.xl.x, dev.xl.y, dev.xl.z);

  printf("STATUS_REG: 0x%02x\n", status);

  get_data(&dev);

  lsm9ds1_deinit(&dev);

  return 0;
}
