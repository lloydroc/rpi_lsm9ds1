#include "../config.h"
#include <stdio.h>

#include <stdint.h>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "error.h"
#include "become_daemon.h"
#include "options.h"
#include "gpio.h"
#include "lsm9ds1.h"

struct options opts;
struct LSM9DS1 dev;

static
void signal_handler(int sig)
{
   lsm9ds1_deinit(&dev);
}

int
get_data(struct options *opts, struct LSM9DS1 *dev)
{
  int timeout;
  struct pollfd pfd;
  int fd, fd_data_file;
  int ret;
  char buf[8];

  printf("%d", GPIO_INTERRUPT_AG);
  return EXIT_SUCCESS;

  if (signal(SIGINT, signal_handler) == SIG_ERR)
    err_output("installing SIGNT signal handler");
  if (signal(SIGTERM, signal_handler) == SIG_ERR)
    err_output("installing SIGTERM signal handler");
  if (signal(SIGKILL, signal_handler) == SIG_ERR)
    err_output("installing SIGKILL signal handler");

  fd_data_file = -1;
  if(opts->data_file != NULL)
    fd_data_file = fileno(opts->data_file);

  timeout = 1000;

  fd = dev->fd_int1_ag_pin;
  pfd.fd = fd;

  pfd.events = POLLPRI;

  for(int i=0;i<1000;i++)
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

    lsm9ds1_ag_read_g(dev);
    lsm9ds1_ag_read_xl(dev);

    if(fd_data_file != -1 && isatty(fd_data_file))
    {
      lsm9ds1_ag_write_terminal(dev);
    }
    else if(fd_data_file != -1)
    {
      lsm9ds1_ag_write_file(dev, opts->data_file, opts->binary);
    }
  }

  close(fd);

  return 1;
}

int
main(int argc, char *argv[])
{
  int fd, ret;

  options_init(&opts);
  ret = options_parse(&opts, argc, argv);
  if(ret || opts.help)
  {
    usage();
    return ret;
  }
  else if(opts.help)
  {
    usage();
    return EXIT_SUCCESS;
  }

  if(lsm9ds1_configure_ag_interrupt(opts.gpio_interrupt_ag, &fd))
  {
    fprintf(stderr, "unable to configure interrupt %d\n", opts.gpio_interrupt_ag);
    return EXIT_FAILURE;
  }

  dev.spidev_ag = "/dev/spidev0.0";
  dev.spi_clk_hz = opts.spi_clk_hz;
  dev.odr = opts.odr;
  dev.fd_int1_ag_pin = fd;

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
    lsm9ds1_configure(&dev);
    lsm9ds1_test(&dev);
  }
  else if(opts.daemon)
  {
    if(opts.data_file == stdout)
      opts.data_file = NULL;
    become_daemon();
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

  ret = lsm9ds1_test(&dev);
  if(ret)
  {
    fprintf(stderr, "Failed %d test cases\n", ret);
    return EXIT_FAILURE;
  }
  else
  {
    if(opts.test)
      return EXIT_SUCCESS;
  }

  get_data(&opts, &dev);

  if(lsm9ds1_unconfigure_ag_interrupt(opts.gpio_interrupt_ag, dev.fd_int1_ag_pin))
  {
    fprintf(stderr, "unable to unconfigure gpio pin %d\n", opts.gpio_interrupt_ag);
    return EXIT_FAILURE;
  }

  lsm9ds1_deinit(&dev);

  return 0;
}

