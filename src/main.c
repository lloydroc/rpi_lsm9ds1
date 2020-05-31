#include "../config.h"
#include <stdio.h>

#include <stdint.h>
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
  int exit_status;
  exit_status = lsm9ds1_deinit(&dev, &opts);
  exit(exit_status);
}

int
main(int argc, char *argv[])
{
  int fd, ret;

  if (signal(SIGINT, signal_handler) == SIG_ERR)
    err_output("installing SIGNT signal handler");
  if (signal(SIGTERM, signal_handler) == SIG_ERR)
    err_output("installing SIGTERM signal handler");

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

  if(opts.spi_dev == 0)
    dev.spidev_ag = "/dev/spidev0.0";
  else if(opts.spi_dev == 1)
    dev.spidev_ag = "/dev/spidev0.1";

  dev.spi_clk_hz = opts.spi_clk_hz;
  dev.odr = opts.odr;
  dev.fd_int1_ag_pin = fd;

  lsm9ds1_init(&dev);

  if(opts.reset)
  {
    lsm9ds1_reset(&dev);
    ret = EXIT_SUCCESS;
    goto cleanup;
  }
  else if(opts.configure)
  {
    lsm9ds1_configure(&dev);
    ret = lsm9ds1_test(&dev);
    goto cleanup;
  }
  else if(opts.daemon)
  {
    if(opts.data_file == stdout)
      opts.data_file = NULL;
    become_daemon();
  }

  ret = lsm9ds1_test(&dev);
  if(ret)
  {
    fprintf(stderr, "Failed %d test cases\n", ret);
    ret = EXIT_FAILURE;
    goto cleanup;
  }
  else
  {
    // skip test since we did it above
    if(opts.test)
      ret = EXIT_SUCCESS;
    goto cleanup;
  }

  lsm9ds1_ag_poll(&dev, &opts);

cleanup:
  ret = lsm9ds1_deinit(&dev, &opts);

  return ret;
}

