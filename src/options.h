#ifndef OPTIONS_H
#define OPTIONS_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "error.h"
#include "lsm9ds1_regs.h"

#ifndef SPI_CLK_HZ
#define SPI_CLK_HZ 8000000
#endif

#ifndef GPIO_INTERRUPT_AG
#define GPIO_INTERRUPT_AG 13
#endif

struct options
{
  int help;
  int reset;
  int test;
  long spi_clk_hz;
  int configure;
  int odr;
  int daemon;
  int gpio_interrupt_ag;
  int spi_dev;
  int interrupt_thresh_g;
  FILE* data_file;
  int fd_socket_udp;
  struct sockaddr_in socket_udp_dest;
  int binary;
};

void
usage(void);

void
options_init(struct options *opts);

int
options_parse(struct options *opts, int argc, char *argv[]);

#endif
