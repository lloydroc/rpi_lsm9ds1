#include "../config.h"
#include <stdio.h>

#include <stdint.h>
#include "options.h"
#include "lsm9ds1.h"

int
main(int argc, char *argv[])
{
  struct options opts;
  struct LSM9DS1 dev;
  uint8_t status = 0;
  uint8_t val = 0;

  options_init(&opts);
  options_parse(&opts, argc, argv);

  dev.spidev_ag = "/dev/spidev0.0";
  dev.spidev_m  = "/dev/spidev0.1";
  dev.spi_clk_hz = opts.spi_clk_hz;

  lsm9ds1_init(&dev);

  if(opts.configure)
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
    thresh.x = 50;
    thresh.y = 50;
    thresh.z = 50;
    lsm9ds1_ag_write_int_g_thresh(&dev, &thresh, 0, 0, 0);
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
    printf("G Data Available\n");
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

  lsm9ds1_deinit(&dev);

  return 0;
}
