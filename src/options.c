#include "options.h"

void
usage(void)
{
  printf("\nUsage: lsm9ds1 [OPTIONS]\n\n");
  printf("OPTIONS:\n\
-h --spi-clk-hz Speed of SPI Clock\n\
-c --configure  Write Configuration\n\
-a --calibrate  Calibrate the LSM9DS\n\
-g --interrupt-thresh-g  Set G Interrupt Thresholds\n\
");
}

void
options_init(struct options *opts)
{
  opts->spi_clk_hz = 8000000;
  opts->configure = 0;
  opts->calibrate = 0;
}

void
options_parse(struct options *opts, int argc, char *argv[])
{
  int c;
  int option_index;
  static struct option long_options[] =
  {
    {"spi-clk-hz",         required_argument, 0, 'h'},
    {"configure",                no_argument, 0,  0},
    {"calibrate",                no_argument, 0,  0},
    {"interrupt-thresh-g",       no_argument, 0,  0},
    {0          ,                  0, 0, 0}
  };

  while(1)
  {
    option_index = 0;
    c = getopt_long_only(argc, argv, "h:cag", long_options, &option_index);

    if(c == -1)
      break;

    switch(c)
    {
    case 0:
      if(strcmp("spi-clk-hz", long_options[option_index].name) == 0)
          opts->spi_clk_hz = atol(optarg);
      else if(strcmp("configure", long_options[option_index].name) == 0)
          opts->configure = 1;
      else if(strcmp("calibrate", long_options[option_index].name) == 0)
          opts->calibrate = 1;
      else if(strcmp("interrupt-thresh-g", long_options[option_index].name) == 0)
          opts->interrupt_thresh_g = 1;

      case 'h':
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
