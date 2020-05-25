#ifndef OPTIONS_H
#define OPTIONS_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "lsm9ds1_regs.h"

struct options
{
  int help;
  int reset;
  long spi_clk_hz;
  int configure;
  int calibrate;
  int odr;
  int interrupt_thresh_g;
};

void
usage(void);

void
options_init(struct options *opts);

int
options_parse(struct options *opts, int argc, char *argv[]);

#endif
