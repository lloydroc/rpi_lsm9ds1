#include "options.h"

void
usage(void)
{
  printf("Usage: lsm9ds1 [OPTIONS]\n\n");
  printf("A command line tool to read data from the ST LSM9DS1.\n");
  printf("After wiring up the lsm9ds1 you MUST run a configuration on it first.\n\n");
  printf("OPTIONS:\n\
-h --help                     Print help\n\
-x --reset                    SW Reset\n\
-t --test                     Perform a test\n\
-z --spi-clk-hz SPEED         Speed of SPI Clock. Default 8000000 Hz\n\
-s --spi-device SPI           Device. Default 0.\n\
-g --rpi-gpio-interrupt GPIO  Interrupt Pin. Default 13.\n\
-c --configure                Write Configuration\n\
-r --odr ODR                  G and XL Sample Frequency in Hz: 14.9, 59.5, 119, 238, 476, 952. Default 14.9 Hz.\n\
-d --daemon                   Run as a Daemon\n\
-f --file FILENAME            Output data to a File\n\
-u --socket-udp HOST:PORT     Output data to a UDP Socket\n\
-b --binary                   Used with the -f and -u options for binary output\n\
");
}

void
options_init(struct options *opts)
{
  opts->reset = 0;
  opts->help = 0;
  opts->test = 0;
  opts->spi_clk_hz = SPI_CLK_HZ;
  opts->spi_dev = 0;
  opts->configure = 0;
  opts->gpio_interrupt_ag = GPIO_INTERRUPT_AG;
  opts->interrupt_thresh_g = 0;
  opts->odr = ODR_AG_14p9_HZ;
  opts->daemon = 0;
  opts->data_file = stdout;
  opts->fd_socket_udp = -1;
}

static int
options_parse_odr(char *optval, int *odr)
{
  *odr = -1;
  if(strcmp("0", optval) == 0)
    *odr = ODR_AG_POWER_DOWN;
  else if(strcmp("14.9", optval) == 0)
    *odr = ODR_AG_14p9_HZ;
  else if(strcmp("59.5", optval) == 0)
    *odr = ODR_AG_59p5_HZ;
  else if(strcmp("119", optval) == 0)
    *odr = ODR_AG_119_HZ;
  else if(strcmp("238", optval) == 0)
    *odr = ODR_AG_238_HZ;
  else if(strcmp("476", optval) == 0)
    *odr = ODR_AG_476_HZ;
  else if(strcmp("952", optval) == 0)
    *odr = ODR_AG_952_HZ;
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
options_open_socket_udp(struct options *opts, char *optarg)
{
  struct hostent *he;
  int sockfd, rc, optval;

  int prt;
  size_t len;
  char *index;
  char host[1024];
  char port[6];

  // separate the host and port by the :

  len = strnlen(optarg, 1024);
  if(len < 3 || len == 1024)
    return 1;

  index = rindex(optarg, ':');
  if(index == NULL)
    return 1;

  strncpy(port, index+1, 6);
  len = strnlen(port, 6);
  if(len < 1 || len == 6)
    return 1;

  prt = atoi(port);
  *index = '\0';

  strncpy(host, optarg, 1024);
  len = strnlen(host, 1024);
  if(len == 0 || len == 1024)
    return 1;

  if ( (he = gethostbyname(host) ) == NULL ) {
      err_output("gethostbyname");
      return 1;
  }

  bzero(&opts->socket_udp_dest, sizeof(struct sockaddr_in));
  memcpy(&opts->socket_udp_dest.sin_addr, he->h_addr_list[0], he->h_length);
  opts->socket_udp_dest.sin_family = AF_INET;
  opts->socket_udp_dest.sin_port = htons(prt);

  sockfd = socket(opts->socket_udp_dest.sin_family, SOCK_DGRAM, 0);
  if(sockfd == -1)
  {
    err_output("socket");
  }

  rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if(rc)
  {
    err_output("setsockopt");
    return 1;
  }

  opts->fd_socket_udp = sockfd;

  return 0;
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
    {"spi-device",         required_argument, 0, 's'},
    {"rpi-gpio-interrupt", required_argument, 0, 'g'},
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
    c = getopt_long_only(argc, argv, "hxtz:cf:u:bdr:s:g:", long_options, &option_index);

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
      {
        opts->spi_clk_hz = atol(optarg);
        opts->configure = 1;
      }
      else if(strcmp("spi-device", long_options[option_index].name) == 0)
      {
        opts->spi_dev = atoi(optarg);
        if(opts->spi_dev > 1)
          ret |= 1;
      }
      else if(strcmp("rpi-gpio-interrupt", long_options[option_index].name) == 0)
        opts->gpio_interrupt_ag = atoi(optarg);
      else if(strcmp("configure", long_options[option_index].name) == 0)
        opts->configure = 1;
      else if(strcmp("odr", long_options[option_index].name) == 0)
      {
        opts->configure = 1;
        ret |= options_parse_odr(optarg, &opts->odr);
      }
      else if(strcmp("file", long_options[option_index].name) == 0)
      {
        opts->data_file = options_open_file_data(optarg);
        ret |= opts->data_file == NULL;
      }
      else if(strcmp("socket-udp", long_options[option_index].name) == 0)
        ret |= options_open_socket_udp(opts, optarg);
      else if(strcmp("binary", long_options[option_index].name) == 0)
        opts->binary = 1;
      else if(strcmp("daemon", long_options[option_index].name) == 0)
        opts->daemon = 1;

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
        opts->configure = 1;
        break;
      case 's':
        opts->spi_dev = atoi(optarg);
        if(opts->spi_dev > 1)
          ret |= 1;
        break;
      case 'c':
        opts->configure = 1;
        break;
      case 'r':
        ret = options_parse_odr(optarg, &opts->odr);
        opts->configure = 1;
        break;
      case 'f':
        opts->data_file = options_open_file_data(optarg);
        ret |= opts->data_file == NULL;
        break;
      case 'u':
        ret |= options_open_socket_udp(opts, optarg);
        break;
      case 'b':
        opts->binary = 1;
        break;
      case 'd':
        opts->daemon = 1;
        break;
      case 'g':
        opts->gpio_interrupt_ag = atoi(optarg);
        break;
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
