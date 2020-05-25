#ifndef OPTIONS_H
#define OPTIONS_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

struct options
{
  int reset;
  long spi_clk_hz;
  int configure;
  int calibrate;
  int interrupt_thresh_g;
};

void
usage(void);

void
options_init(struct options *opts);

void
options_parse(struct options *opts, int argc, char *argv[]);

#endif
