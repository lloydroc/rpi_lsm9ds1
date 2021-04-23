#include "spi.h"

int
spi_init(struct SPI* spi, char device[], int speed_hz, uint8_t mode, uint8_t bits_per_word, uint8_t chip_select)
{
  spi->speed_hz = speed_hz;
  spi->mode = mode;
  spi->bits_per_word = bits_per_word;
  spi->chip_select = chip_select;
  spi->fd = spi_open(device, mode, speed_hz, O_RDWR);

#ifdef HAVE_LINUX_SPI_SPIDEV_H
  // read linux/spi/spidev.h for good docs on this structure
  spi->transfer.tx_buf = (unsigned long) NULL; // the buffer for sending data
  spi->transfer.rx_buf = (unsigned long) NULL; // the buffer for receiving data
  spi->transfer.len = 0;                       // the length of buffer
  spi->transfer.speed_hz = speed_hz;
  spi->transfer.bits_per_word = bits_per_word; // bits per word
  spi->transfer.delay_usecs = 0;               // delay in us
  spi->transfer.cs_change = spi->chip_select;  // affects chip select after transfer
  spi->transfer.tx_nbits = 0;                  // no. bits for writing (default 0)
  spi->transfer.rx_nbits = 0;                  // no. bits for reading (default 0)
  spi->transfer.pad = 0;                       // interbyte delay - check version
#endif

  return spi->fd;
}

int
spi_destroy(struct SPI* spi)
{
  return close(spi->fd);
}

int
spi_open(const char spidev[], int mode, int speed_hz, int oflag)
{
  int fd;

  fd = 0;

#ifdef HAVE_LINUX_SPI_SPIDEV_H
  // The following calls set up the SPI bus properties
  if ((fd = open(spidev, oflag))<0){
    err_output("SPI Error: Can't open device.");
    return -1;
  }
  if(oflag == O_WRONLY || oflag == O_RDWR) {
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode)==-1){
      err_output("SPI: Can't set SPI write mode.");
      return -1;
    }
  }
  if(oflag == O_RDONLY || oflag == O_RDWR) {
    if (ioctl(fd, SPI_IOC_RD_MODE, &mode)==-1){
      err_output("SPI: Can't get SPI read mode.");
      return -1;
    }
  }
  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz)==-1){
    err_output("SPI: Can't set max speed Hz");
    return -1;
  }
  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed_hz)==-1){
    err_output("SPI: Can't get max speed Hz.");
    return -1;
  }
#endif

  return fd;
}

int
spi_transfer(struct SPI* spi, uint8_t send[], uint8_t rec[], int len)
{
  int status;
  status = 0;

#ifdef HAVE_LINUX_SPI_SPIDEV_H
  spi->transfer.tx_buf = (unsigned long) send;
  spi->transfer.rx_buf = (unsigned long) rec;
  spi->transfer.len = len;

  // send the SPI message (all of the above fields, inc. buffers)
  // 1 is the number of spi_ioc_transfer structs to send
  int status = ioctl(spi->fd, SPI_IOC_MESSAGE(1), &spi->transfer);
  /*
   * cannot find documentation on the return code and < 0 can be success
   if (status < 0) {
   err_output("SPI: SPI_IOC_MESSAGE Failed");
   }
  */
#endif
  return status && 0;
}
