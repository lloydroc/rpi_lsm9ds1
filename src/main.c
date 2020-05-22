#include "../config.h"
#include <stdio.h>

#include <stdint.h>
#include "lsm9ds1.h"

int
main(int argc, char *argv[])
{
  struct LSM9DS1 dev;

  dev.spidev_ag = "/dev/spidev0.0";
  dev.spidev_m  = "/dev/spidev0.1";
  //dev.spi_clk_hz = 8000000;
  dev.spi_clk_hz = 8000;

  lsm9ds1_init(&dev);

  uint8_t val = 0;
  uint16_t data = 0;
  lsm9ds1_ag_read(&dev, WHO_AM_I, &val);
  printf("WHO_AM_I: 0x%02x\n", val);

  lsm9ds1_ag_read(&dev, STATUS_REG, &val);
  printf("STATUS_REG: 0x%02x\n", val);

  lsm9ds1_ag_read(&dev, FIFO_CTRL, &val);
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
    printf("FIFO is not full\n");

  printf("FIFO has %d samples\n", val & FSS);

  lsm9ds1_ag_read2(&dev, OUT_X_XL, &data);
  printf("OUT_X_XL: 0x%02x\n", data);

  lsm9ds1_ag_read2(&dev, OUT_Y_XL, &data);
  printf("OUT_Y_XL: 0x%02x\n", data);

  lsm9ds1_ag_read2(&dev, OUT_Z_XL, &data);
  printf("OUT_Z_XL: 0x%02x\n", data);

  lsm9ds1_ag_read2(&dev, OUT_X_G, &data);
  printf("OUT_X_G: 0x%02x\n", data);

  lsm9ds1_ag_read2(&dev, OUT_Y_G, &data);
  printf("OUT_Y_G: 0x%02x\n", data);

  lsm9ds1_ag_read2(&dev, OUT_Z_G, &data);
  printf("OUT_Z_G: 0x%02x\n", data);

  lsm9ds1_ag_read2(&dev, OUT_TEMP_L, &data);
  printf("OUT_TEMP_L: 0x%02x\n", data);





  if(argc > 1)
  {
    printf("configuring LSM9DS1\n");
    lsm9ds1_configure(&dev);
  }

  lsm9ds1_deinit(&dev);

  return 0;
}
