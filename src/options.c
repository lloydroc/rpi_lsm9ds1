#include "options.h"

// tells error.c when we print to use stdout, stderr, or syslog
int use_syslog = 0;

void
usage(void)
{
  printf("Usage: lsm9ds1 [OPTIONS]\n\n");
  printf("Version: %s\n", VERSION);
  printf("A command line tool to read data from the ST LSM9DS1.\n");
  printf("After wiring up the lsm9ds1 you MUST run a configuration on it first.\n\n");
  printf("OPTIONS:\n\
-h --help                     Print help\n\
-x --reset                    SW Reset\n\
-t --test                     Perform a test\n\
-z --spi-clk-hz SPEED         Speed of SPI Clock. Default 8000000 Hz\n\
-s --spi-device SPI           Device. Default 0.\n\
   --ag-gpio-interrupt GPIO   Interrupt Pin for G and XL. Default 13.\n\
   --m-gpio-interrupt GPIO    Interrupt Pin for M. Default 6.\n\
-c --configure                Write Configuration\n\
-r --odr-ag ODR               G and XL Sample Frequency in Hz: 14.9, 59.5, 119, 238, 476, 952. Default 14.9 Hz.\n\
-m --odr-m ODR                M Sample Frequency in Hz: 0.625, 1.25, 2.5, 5, 10, 20, 40, 80. Default 10 Hz.\n\
-d --daemon                   Run as a Daemon\n\
-f --file FILENAME            Output data to a File\n\
-u --socket-udp HOST:PORT     Output data to a UDP Socket\n\
-b --binary                   Used with the -f and -u options for binary output\n\
   --silent                   Do not print sensor readings to standard output but print other warnings and errors\n\
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
  opts->gpio_interrupt_m = GPIO_INTERRUPT_M;
  opts->odr_ag = ODR_AG_14p9_HZ;
  opts->odr_m = ODR_M_10_HZ;
  opts->daemon = 0;
  opts->data_file = NULL;
  opts->fd_socket_udp = -1;
  opts->silent = 0;
}

static int
options_parse_odr_ag(char *optval, int *odr_ag)
{
  *odr_ag = -1;
  if(strcmp("0", optval) == 0)
    *odr_ag = ODR_AG_POWER_DOWN;
  else if(strcmp("14.9", optval) == 0)
    *odr_ag = ODR_AG_14p9_HZ;
  else if(strcmp("59.5", optval) == 0)
    *odr_ag = ODR_AG_59p5_HZ;
  else if(strcmp("119", optval) == 0)
    *odr_ag = ODR_AG_119_HZ;
  else if(strcmp("238", optval) == 0)
    *odr_ag = ODR_AG_238_HZ;
  else if(strcmp("476", optval) == 0)
    *odr_ag = ODR_AG_476_HZ;
  else if(strcmp("952", optval) == 0)
    *odr_ag = ODR_AG_952_HZ;
  return *odr_ag == -1;
}

static int
options_parse_odr_m(char *optval, int *odr_m)
{
  *odr_m = -1;
  if(strcmp("0.625", optval) == 0)
    *odr_m = ODR_M_0p625_HZ;
  else if(strcmp("1.25", optval) == 0)
    *odr_m = ODR_M_1p25_HZ;
  else if(strcmp("2.5", optval) == 0)
    *odr_m = ODR_M_2p5_HZ;
  else if(strcmp("5", optval) == 0)
    *odr_m = ODR_M_5_HZ;
  else if(strcmp("10", optval) == 0)
    *odr_m = ODR_M_10_HZ;
  else if(strcmp("20", optval) == 0)
    *odr_m = ODR_M_20_HZ;
  else if(strcmp("40", optval) == 0)
    *odr_m = ODR_M_40_HZ;
  else if(strcmp("80", optval) == 0)
    *odr_m = ODR_M_80_HZ;
  return *odr_m == -1;
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
    {"help",                     no_argument, 0, 'h'},
    {"reset",                    no_argument, 0, 'x'},
    {"spi-clk-hz",         required_argument, 0, 'z'},
    {"test",                     no_argument, 0, 't'},
    {"spi-device",         required_argument, 0, 's'},
    {"ag-gpio-interrupt", required_argument, 0,   0},
    {"configure",                no_argument, 0, 'c'},
    {"odr-ag",             required_argument, 0, 'r'},
    {"odr-m",             required_argument, 0,  'm'},
    {"daemon",                   no_argument, 0, 'd'},
    {"file",               required_argument, 0, 'f'},
    {"socket-udp",         required_argument, 0, 'u'},
    {"binary",                   no_argument, 0, 'b'},
    {"silent",                   no_argument, 0,   0},
    {0,                                    0, 0,   0}
  };

  while(1)
  {
    option_index = 0;
    c = getopt_long_only(argc, argv, "hxtz:cf:u:bdr:m:s:", long_options, &option_index);

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
      else if(strcmp("ag-gpio-interrupt", long_options[option_index].name) == 0)
        opts->gpio_interrupt_ag = atoi(optarg);
      else if(strcmp("m-gpio-interrupt", long_options[option_index].name) == 0)
        opts->gpio_interrupt_m = atoi(optarg);
      else if(strcmp("configure", long_options[option_index].name) == 0)
        opts->configure = 1;
      else if(strcmp("odr_ag", long_options[option_index].name) == 0)
      {
        opts->configure = 1;
        ret |= options_parse_odr_ag(optarg, &opts->odr_ag);
      }
      else if(strcmp("odr_m", long_options[option_index].name) == 0)
      {
        opts->configure = 1;
        ret |= options_parse_odr_m(optarg, &opts->odr_m);
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
      else if(strcmp("silent", long_options[option_index].name) == 0)
        opts->silent = 1;
      else
        opts->help = 1;
      break;

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
        ret = options_parse_odr_ag(optarg, &opts->odr_ag);
        opts->configure = 1;
        break;
      case 'm':
        ret = options_parse_odr_m(optarg, &opts->odr_m);
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
    }
  }

  // this option sets whether we log to syslog or not
  // and should checked first
  if(opts->daemon)
  {
    use_syslog = 1;
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
