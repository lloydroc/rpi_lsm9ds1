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
  int ret;

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

#ifndef HAVE_LINUX_SPI_SPIDEV_H
  fprintf(stderr, "Linux headers for SPI not found ... exiting\n");
  return EXIT_FAILURE;
#endif

  if(lsm9ds1_configure_interrupt(opts.gpio_interrupt_ag, &dev.fd_int1_ag_pin))
  {
    fprintf(stderr, "unable to configure AG interrupt %d\n", opts.gpio_interrupt_ag);
    return EXIT_FAILURE;
  }

  if(lsm9ds1_configure_interrupt(opts.gpio_interrupt_m, &dev.fd_int1_m_pin))
  {
    fprintf(stderr, "unable to configure M interrupt %d\n", opts.gpio_interrupt_m);
    return EXIT_FAILURE;
  }

  if(opts.spi_dev == 0)
  {
    dev.spidev_ag = "/dev/spidev0.0";
    dev.spidev_m = "/dev/spidev0.1";
  }
  else if(opts.spi_dev == 1)
  {
    dev.spidev_ag = "/dev/spidev1.1";
    dev.spidev_m = "/dev/spidev1.1";
  }

  dev.spi_clk_hz = opts.spi_clk_hz;
  dev.odr_ag = opts.odr_ag;
  dev.odr_m = opts.odr_m;

  lsm9ds1_init(&dev);

  if(opts.help)
  {
    usage();
    return 0;
  }
  else if(opts.reset)
  {
    ret = lsm9ds1_reset(&dev);
    if(ret)
    {
      fprintf(stderr, "failed to reset\n");
    }
    goto cleanup;
  }
  else if(opts.configure)
  {
    ret = lsm9ds1_configure(&dev);
    if(ret == 1)
    {
      fprintf(stderr, "unable to configure AG sensor\n");
      goto cleanup;
    }
    else if(ret == 2)
    {
      fprintf(stderr, "unable to configure M sensor\n");
      goto cleanup;
    }
    ret = lsm9ds1_test(&dev);
    goto cleanup;
  }
  else if(opts.daemon)
  {
    if(opts.data_file == stdout)
      opts.data_file = NULL;
    become_daemon();
    openlog("lsm9ds1", 0, LOG_DAEMON);
    if(write_pidfile("/run/lsm9ds1.pid"))
    {
      errno_output("unable to write pid file\n");
    }
    info_output("daemon started pid=%ld", getpid());
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
    {
      ret = EXIT_SUCCESS;
      goto cleanup;
    }
  }

  lsm9ds1_ag_poll(&dev, &opts);

cleanup:
  ret = lsm9ds1_deinit(&dev, &opts);

  return ret;
}
