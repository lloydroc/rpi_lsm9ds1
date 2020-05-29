#include "options.h"

void
usage(void)
{
  printf("Usage: lsm9ds1 [OPTIONS]\n\n");
  printf("OPTIONS:\n\
-h --help Print help\n\
-x --reset SW Reset\n\
-t --test perform a test\n\
-z --spi-clk-hz SPEED Speed of SPI Clock\n\
-c --configure  Write Configuration\n\
-r --odr ODR Sample Frequency for G and XL. Choose: 14.9, 59.5, 119, 238, 476 or 952\n\
-d --daemon Run as a Deamon\n\
-f --file FILENAME Output Data to a File\n\
-u --socket-udp HOST:PORT Output data to a UDP Socket\n\
-b --binary Used with the -f and -u options for binary output\n\
-g --interrupt-thresh-g  Set G Interrupt Thresholds\n\
");
}

void
options_init(struct options *opts)
{
  opts->reset = 0;
  opts->help = 0;
  opts->test = 0;
  opts->spi_clk_hz = SPI_CLK_HZ;
  opts->configure = 0;
  opts->gpio_interrupt_ag = GPIO_INTERRUPT_AG;
  opts->interrupt_thresh_g = 0;
  opts->odr = 1;
  opts->daemon = 0;
  opts->data_file = stdout;
}

static int
options_parse_odr(char *optval, int *odr)
{
  *odr = -1;
  if(strcmp("0", optval) == 0)
    *odr = ODR_POWER_DOWN;
  else if(strcmp("14.9", optval) == 0)
    *odr = ODR_14p9_HZ;
  else if(strcmp("59.5", optval) == 0)
    *odr = ODR_59p5_HZ;
  else if(strcmp("119", optval) == 0)
    *odr = ODR_119_HZ;
  else if(strcmp("238", optval) == 0)
    *odr = ODR_238_HZ;
  else if(strcmp("476", optval) == 0)
    *odr = ODR_476_HZ;
  else if(strcmp("952", optval) == 0)
    *odr = ODR_952_HZ;
  return *odr == -1;
}

static FILE*
options_open_file_data(char *optarg)
{
  FILE* file = fopen(optarg, "w");
  if(file == NULL)
  {
    err_output("unable to open file %s", optarg);
  }
  return file;
}

static
int
options_open_socket_udp(char* optarg)
{
  return -1;
}

int
options_parse(struct options *opts, int argc, char *argv[])
{
  int c;
  int option_index;
  int ret = 0;
  static struct option long_options[] =
  {
    {"help",                     no_argument, 0,   0},
    {"reset",                    no_argument, 0,   0},
    {"spi-clk-hz",         required_argument, 0, 'h'},
    {"configure",                no_argument, 0,   0},
    {"odr",                required_argument, 0, 'r'},
    {"deamon",                   no_argument, 0, 'd'},
    {"file",               required_argument, 0, 'f'},
    {"socket-udp",         required_argument, 0, 'u'},
    {"binary",                   no_argument, 0, 'u'},
    {"interrupt-thresh-g",       no_argument, 0,   0},
    {0,                                    0, 0,   0}
  };

  while(1)
  {
    option_index = 0;
    c = getopt_long_only(argc, argv, "hxtz:cf:u:bdr:g", long_options, &option_index);

    if(c == -1)
      break;

    switch(c)
    {
    case 0:
      if(strcmp("help", long_options[option_index].name) == 0)
        opts->help = 1;
      else if(strcmp("reset", long_options[option_index].name) == 0)
        opts->reset = 1;
      else if(strcmp("test", long_options[option_index].name) == 0)
        opts->test = 1;
      else if(strcmp("spi-clk-hz", long_options[option_index].name) == 0)
        opts->spi_clk_hz = atol(optarg);
      else if(strcmp("configure", long_options[option_index].name) == 0)
        opts->configure = 1;
      else if(strcmp("odr", long_options[option_index].name) == 0)
      {
        ret |= options_parse_odr(optarg, &opts->odr);
      }
      else if(strcmp("file", long_options[option_index].name) == 0)
      {
        opts->data_file = options_open_file_data(optarg);
        ret |= opts->data_file == NULL;
      }
      else if(strcmp("socket-udp", long_options[option_index].name) == 0)
      {
        opts->fd_socket_udp = options_open_socket_udp(optarg);
        ret |= opts->fd_socket_udp == -1;
      }
      else if(strcmp("binary", long_options[option_index].name) == 0)
        opts->binary = 1;
      else if(strcmp("daemon", long_options[option_index].name) == 0)
        opts->daemon = 1;
      else if(strcmp("interrupt-thresh-g", long_options[option_index].name) == 0)
         opts->interrupt_thresh_g = 1;

      case 'h':
        opts->help = 1;
        break;
      case 'x':
        opts->reset = 1;
        break;
      case 't':
        opts->test = 1;
        break;
      case 'z':
        opts->spi_clk_hz = atol(optarg);
        break;
      case 'c':
        opts->configure = 1;
        break;
      case 'r':
         ret = options_parse_odr(optarg, &opts->odr);
         break;
      case 'f':
         opts->data_file = options_open_file_data(optarg);
         ret |= opts->data_file == NULL;
         break;
      case 'u':
         opts->fd_socket_udp = options_open_socket_udp(optarg);
         ret |= opts->fd_socket_udp == -1;
         break;
      case 'b':
         opts->binary = 1;
         break;
      case 'd':
         opts->daemon = 1;
         break;
      case 'g':
        opts->interrupt_thresh_g = 1;
        break;

      default:
        fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
    }
  }

  if (optind < argc)
  {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    puts("");
  }

  return ret;
}
