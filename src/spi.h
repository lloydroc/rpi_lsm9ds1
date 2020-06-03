#ifndef SPI_INCLUDED
#define SPI_INCLUDED

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include "error.h"

// See pinout.xyz showing physical pins
// spidev0.0
// CE0 = Pin 24, CE1 = Pin 26, MOSI = Pin 19, MISO = Pin 21, SCLK = Pin 23
// spidev0.1
// CE0 = Pin 12, CE1 = Pin 11, MOSI = Pin 38, MISO = Pin 35, SCLK = Pin 40
#define SPIPATH "/dev/spidev"

struct SPI {
  char* device;
  unsigned int speed_hz;
  uint8_t mode;
  uint8_t bits_per_word;
  uint8_t chip_select;
  int fd;
  struct spi_ioc_transfer transfer;
};

int
spi_init(struct SPI* spi, char device[], int speed_hz, uint8_t mode, uint8_t bits_per_word, uint8_t chip_select);

int
spi_destroy(struct SPI* spi);

// Open the SPI Device
// dev is for example 0.0 in which /dev/spidev will be prepended
// mode is SPIMODE type see
//   https://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus#Mode_numbers
// speed_hz is the speed for the writing and reading
// oflag is O_RDONLY, O_WRONLY, O_RDWR defined in fnctl.h
// returns a file descriptor to the device or -1 on error
// file descriptor is closed as a normal file would be with close
int
spi_open(const char dev[], int mode, int speed_hz, int oflag);

int
spi_transfer(struct SPI* spi, uint8_t send[], uint8_t rec[], int len);

#endif
