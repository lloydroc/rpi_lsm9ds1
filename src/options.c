#include "options.h"

void
usage(void)
{
  printf("\nUsage: lsm9ds1 [OPTIONS]\n\n");
  printf("OPTIONS:\n\
-h --help Print help\n\
-r --reset SW Reset\n\
-z --spi-clk-hz Speed of SPI Clock\n\
-c --configure  Write Configuration\n\
-a --calibrate  Calibrate the LSM9DS\n\
-r --odr ODR Sample Frequency for G and XL\n\
-g --interrupt-thresh-g  Set G Interrupt Thresholds\n\
");
}

void
options_init(struct options *opts)
{
  opts->reset = 0;
  opts->help = 0;
  opts->spi_clk_hz = 8000000;
  opts->configure = 0;
  opts->calibrate = 0;
  opts->interrupt_thresh_g = 0;
  opts->odr = 1;
}

int
options_parse_odr(char *optval)
{
  int odr = -1;
  if(strcmp("0", optval) == 0)
    odr = 0;
  else if(strcmp("15.9", optval) == 0)
    odr = 1;
  else if(strcmp("59.5", optval) == 0)
    odr = 2;
  else if(strcmp("119", optval) == 0)
    odr = 3;
  else if(strcmp("238", optval) == 0)
    odr = 4;
  else if(strcmp("476", optval) == 0)
    odr = 5;
  else if(strcmp("952", optval) == 0)
    odr = 6;
  return odr;
}

void
options_parse(struct options *opts, int argc, char *argv[])
{
  int c;
  int option_index;
  static struct option long_options[] =
  {
    {"help",                     no_argument, 0,  0},
    {"reset",                    no_argument, 0,  0},
    {"spi-clk-hz",         required_argument, 0, 'h'},
    {"configure",                no_argument, 0,  0},
    {"calibrate",                no_argument, 0,  0},
    {"odr",                required_argument, 0,  'r'},
    {"interrupt-thresh-g",       no_argument, 0,  0},
    {0          ,                  0, 0, 0}
  };

  while(1)
  {
    option_index = 0;
    c = getopt_long_only(argc, argv, "hrz:carg", long_options, &option_index);

    if(c == -1)
      break;

    switch(c)
    {
    case 0:
      if(strcmp("help", long_options[option_index].name) == 0)
        opts->help = 1;
      else if(strcmp("reset", long_options[option_index].name) == 0)
        opts->reset = 1;
      else if(strcmp("spi-clk-hz", long_options[option_index].name) == 0)
        opts->spi_clk_hz = atol(optarg);
      else if(strcmp("configure", long_options[option_index].name) == 0)
        opts->configure = 1;
      else if(strcmp("calibrate", long_options[option_index].name) == 0)
        opts->calibrate = 1;
      else if(strcmp("odr", long_options[option_index].name) == 0)
      {
        opts->odr = options_parse_odr(optarg);
      }
      else if(strcmp("interrupt-thresh-g", long_options[option_index].name) == 0)
          opts->interrupt_thresh_g = 1;

      case 'h':
        usage();
        break;
      case 'r':
        opts->reset = 1;
        break;
      case 'z':
        opts->spi_clk_hz = atol(optarg);
        break;
      case 'c':
        opts->configure = 1;
        break;
      case 'a':
        opts->calibrate = 1;
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
}
