# LSM9DS1

A command line tool to interact with the ST LSM9DS1 on the Raspberry Pi.

# Features

* Output Accelerometer, Gyroscope and Magnetometer sensor data to the terminal.
* Write Accelerometer, Gyroscope and Magnetometer sensor data to a file for post processing.
* Send Accelerometer, Gyroscope and Magnetomter Data to a UDP Socket for near-real time processing.
* Run as a daemon process in the background.
* Low CPU and Power consumption due to Interrupt Driven Asynchronous I/O.

# Getting Started

See the downloading and installation below. Once installed here are the options:

```
$ lsm9ds1 -h
Usage: lsm9ds1 [OPTIONS]

Version: 2.0
A command line tool to read data from the ST LSM9DS1.
After wiring up the lsm9ds1 you MUST run a configuration on it first.

OPTIONS:
-h --help                     Print help
-x --reset                    SW Reset
-t --test                     Perform a test
-z --spi-clk-hz SPEED         Speed of SPI Clock. Default 8000000 Hz
-s --spi-device SPI           Device. Default 0.
   --ag-gpio-interrupt GPIO   Interrupt Pin for G and XL. Default 13.
   --m-gpio-interrupt GPIO    Interrupt Pin for M. Default 6.
-c --configure                Write Configuration
-r --odr-ag ODR               G and XL Sample Frequency in Hz: 14.9, 59.5, 119, 238, 476, 952. Default 14.9 Hz.
-m --odr-m ODR                M Sample Frequency in Hz: 0.625, 1.25, 2.5, 5, 10, 20, 40, 80. Default 10 Hz.
-d --daemon                   Run as a Daemon
-f --file FILENAME            Output data to a File
-u --socket-udp HOST:PORT     Output data to a UDP Socket
-b --binary                   Used with the -f and -u options for binary output
```

We separate the configuration of the LSM9DS1 and sensor sampling. This means that we don't write to the configuration each time, it is a separate step that must be specified in an option. Here is how that looks when we collect samples.

```
$ lsm9ds1 -x # software reset LSM9DS1
$ lsm9ds1 -r 59.5 -m 40 -c # configure LSM9DS1 with 59.5 Hz for XL,G and 40Hz for M
$ lsm9ds1 # see the readings, type CTRL-C to stop
```

# Downloading

Download the distribution tarball. The source code in this repo should be used if you want to develop code on it. See the Contributing section below for changing the source code.

```
wget https://lloydrochester.com/code/rpi_lsm9ds1-2.0.tar.gz
tar zxf rpi_lsm9ds1-2.0.tar.gz
```

# Installing

We have the typical Autotools installation flow. We will compile the code from source and install it on the system.

Firstly, if we're running as non-root the user running the program will need access to SPI and GPIO. Use `raspi-config` to enable SPI. Then add the user to the following groups:

```
$ usermod -a -G gpio spi pi # assumes the pi user
```

Note, this command will not take effect until the user logs in and out again. You can verify using the `groups` command.

After downloading the tarball and extracting - assuming you're in the `rpi_lsm9ds1` folder:

```
$ cd rpi_lsm9ds1-2.0
$ ./configure
$ make
$ sudo make install
```

The `sudo make install` will install it for all users. They would need to be part of the `spi` and `gpio` groups to run the `lsm9ds1` binary as well. This step can be skipped an you can just run the `./src/lsm9ds1` binary directly.

# Uninstalling

You can uninstall by going into the directory where it was installed and typing a:

```
sudo make uninstall
```

# Wiring


| LSM9DS1 Pin | LSM9DS1 Description                     | RPI Pin | RPI Description            | RPI BCM Pin |
|-------------|-----------------------------------------|---------|----------------------------|-------------|
| VDDIO       | Power Supply for I/O Pins               | 17      | +3.3V Power                |             |
| GND         | Ground                                  | 25      | Ground                     |             |
| SCL         | SPI Serial Clock                        | 23      | SPI0 Serial Clock          | 11          |
| SDA         | SPI Serial Data Input                   | 19      | SPI0 Master Out / Slave In | 10          |
| CS_A/G      | Chip Select for A/G                     | 24      | SPI0 Chip Enable           | 8           |
| SDO_A/G     | SPI Serial Data Output for A/G          | 21      | SPI0 Master In / Slave Out | 9           |
| SDO_M       | SPI Serial Data Output for M            | 21      | SPI0 Master In / Slave Out | 9           |
| INT1_A/G    | Accelerometer and Gyrometer Interrupt 1 | 33      | GPIO Pin 13                | 13          |
| INT_M       | Magnetometer Interrupt                  | 31      | GPIO Pin 6                 | 6           |

The wiring for the interrupt pins can be changed with the `--ag-gpio-interrupt` and `--m-gpio-interrupt` options by specifying the header pin number. The SPI device can be changed using the `-s` or `--spi-device` and specifying the SPI device. If you do this ensure you have both chip selects enabled.

# Data Files

When using the `-f` or `--file` flags the output format will be CSV with rows looking like:

```
1619557677.618180,128,-278,-124,4270,-73,16222,-11953,-10708,-3067
```

Here is a description of each column:
* Unix Epoch in Seconds with fractions seconds
* Gyrometer X - Raw 16 bit 2's compliment value
* Gyrometer Y - Raw 16 bit 2's compliment value
* Gyrometer Z - Raw 16 bit 2's compliment value
* Accelerometer X - Raw 16 bit 2's compliment value
* Accelerometer Y - Raw 16 bit 2's compliment value
* Accelerometer Z - Raw 16 bit 2's compliment value
* Magnetometer X - Raw 16 bit 2's compliment value
* Magnetometer Y - Raw 16 bit 2's compliment value
* Magnetometer Z - Raw 16 bit 2's compliment value

If we use the `-b` or `--binary` option along with the file options we will have the following binary content in a file. This same content can be sent out a UDP socket.

* 4-bytes of Unix Epoch in Seconds
* 4-bytes of microseconds of the Unix Epoch
* 2-bytes with 2's compliment Gyrometer X
* 2-bytes with 2's compliment Gyrometer Y
* 2-bytes with 2's compliment Gyrometer Z
* 2-bytes with 2's compliment Accelerometer X
* 2-bytes with 2's compliment Accelerometer Y
* 2-bytes with 2's compliment Accelerometer Z
* 2-bytes with 2's compliment Magnetometer X
* 2-bytes with 2's compliment Magnetometer Y
* 2-bytes with 2's compliment Magnetometer Z

# UDP Sockets

We can send the data out a UDP socket by specifying a `-u HOST:PORT`. An example would be `lloydrochester.com:5000` or `45.79.9.60:5000`. Each time we get a sample, specified by the rate `-r` option, a UDP packet will be sent out this port and host pair. The format will be the binary format specified when sending to a data file. You can both write a file and send out UDP packets.

# Daemon

When we specify the `-d` option `lsm9ds1` will detach from the terminal and become a long running process with no controlling terminal and become a daemon. Check the `top` command to see the running process and CPU usage. It's assumed the daemon will be run with either and output to a file or UDP socker or both.

# Contributing

Contributing is highly encouraged. Would like to add more configuration options, and other ways to use the tool. Any feedback would be much appreciated. Please file issues or PRs on Github.

First install the GNU autotools:

```
$ sudo apt-get install -y autotools-dev autoconf
```

Then clone the project, `cd` into the directory and run the following:

```
$ ./autogen.sh
$ ./configure
$ make
$ ./src/lsm9ds1 -h
```

The `./configure` script will create the makefiles.
