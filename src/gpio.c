#include "gpio.h"
#include "error.h"

int
gpio_exists()
{
  return access(GPIO_PATH, F_OK);
}

int
gpio_permissions_valid()
{
    uid_t uid = geteuid();
    gid_t groups[1024];
    struct stat statbuf;
    int ret, ngroups;

    ngroups = getgroups(1024, groups);
    if(ngroups == -1)
        err_exit("unable to get groups");

    ret = stat(GPIO_EXPORT_PATH, &statbuf);
    if(ret)
        err_exit("unable to GPIO on %s\n", GPIO_EXPORT_PATH);

    for(int i=0;i<ngroups;i++)
    {
        // user is probably root, but definitely owns the file!
        if(uid == statbuf.st_uid && (statbuf.st_mode & S_IWUSR))
            return 2;

        // user is in file's group and has write permissions
        if(groups[i] == statbuf.st_gid && (statbuf.st_mode & S_IWGRP))
            return 2;
    }

    return -1;
}

int
gpio_valid(int gpio)
{
    return 0;
}

static int
filter(const struct dirent *name)
{
    size_t len;
    const char *fname;
    fname = name->d_name;
    len = strlen(fname);

    if(len < 5 || len > 6) return 0;
    if(strncmp("gpio", fname, 4)) return 0;
    if(isdigit(fname[4]) == 0) return 0;

    return 1;
}

int
gpio_get_exports(int inputs[], int outputs[], int *num_inputs, int *num_outputs)
{
    struct dirent **exports;
    char *gpio_ptr;
    int gpio;
    int dir;
    int n, no, ni;

    n = scandir(GPIO_PATH, &exports, filter, alphasort);
    if(n == -1)
    {
        err_output("unable to list directory %s", GPIO_PATH);
        return n;
    }

    ni= 0;
    no = 0;

    while(n--)
    {
        gpio_ptr = exports[n]->d_name;
        gpio = atoi(gpio_ptr+4);
        gpio_get_direction(gpio, &dir);
        if(dir == 1)
        {
          inputs[ni++] = gpio;
        }
        else if(dir == 0)
        {
          outputs[no++] = gpio;
        }

        free(exports[n]);
    }
    free(exports);
    *num_inputs = ni;
    *num_outputs = no;

    return 0;
}

// echo 4 > /sys/class/gpio/export
int
gpio_export(int gpio)
{
    int fd;
    char path[64];
    char val[3];

    sprintf(path,GPIO_EXPORT_PATH);
    sprintf(val,"%d",gpio);
    printf("gpio_export: %s %s\n",path,val);
    fd = open(path,O_WRONLY);
    if(fd == -1)
    {
        fprintf(stderr,"Exception exporting GPIO %d",gpio);
        perror("GPIO: Unable to open");
        close(fd);
        return -1;
    }

    if(write(fd,val,strlen(val))<0)
    {
        perror("GPIO: Unable to set output");
        close(fd);
        return -1;
    }
    close(fd);

    sprintf(path,GPIO_VALUE_PATH,gpio);
    fd = 1 << 10;
    while(access(path, F_OK) == -1)
    {
      if(fd-- < 0)
      {
        return -2;
      }
    }
    return 0;
}

static int
gpio_check_direction_permissions_bug(char *path)
{

  int ret;
  struct stat statbuf;
  int canwrite = 0;
  int MAXITER = 1 << 16;
  uid_t uid = geteuid();
  do
  {
    ret = stat(path, &statbuf);
    if(ret)
    {
        err_exit("unable to stat %s\n",path);
    }

    if(uid == statbuf.st_uid && (statbuf.st_mode & S_IWUSR))
        canwrite = 1;
    else if(statbuf.st_mode & S_IWGRP)
        canwrite = 1;
  } while(!canwrite | (--MAXITER < 0));

  if(MAXITER < 0)
    err_exit("unable to get write permissions on %s\n", path);

  return 0;
}


// cat /sys/class/gpio/gpio4/direction
int
gpio_get_direction(int gpio, int *input)
{
    char path[33];
    char *ret;
    FILE* dir;
    char direction[4];

    sprintf(path,GPIO_DIRECTION_PATH,gpio);
    if(gpio_check_direction_permissions_bug(path))
    {
        err_exit("unable to get write permissions on %s\n", path);
        return 1;
    }

    printf("gpio_get_direction: %s\n",path);
    dir = fopen(path,"r");
    if(dir == NULL)
    {
        err_output("Exception opening %s\n", path);
        fclose(dir);
        return -1;
    }

    ret = fgets(direction, 4, dir);
    if(ret == NULL)
    {
        err_output("GPIO: Unable to get direction from %s\n", path);
        fclose(dir);
        return -1;
    }
    fclose(dir);

    if(strcmp("in", direction) == 0) *input = 1;
    else if(strcmp("out", direction) == 0) *input = 0;
    else return -1;

    return 0;
}

// echo out > /sys/class/gpio/gpio4/direction
int
gpio_set_direction(int gpio, int input)
{
    char path[33];
    char val[4];
    int fd;

    sprintf(path,GPIO_DIRECTION_PATH,gpio);
    if(gpio_check_direction_permissions_bug(path))
    {
        err_exit("unable to get write permissions on %s\n", path);
        return 1;
    }

    if(input)
      sprintf(val,"in");
    else
      sprintf(val,"out");

    printf("gpio_set_direction: %s %s\n",path,val);
    fd = open(path,O_WRONLY);
    if(fd == -1)
    {
        err_output("Exception opening %s\n", path);
        close(fd);
        return -1;
    }

    if(write(fd,val,strlen(val))<0)
    {
        err_output("GPIO: Unable to set direction");
        close(fd);
        return -1;
    }
    return close(fd);
}

// open file /sys/class/gpio/gpio4/value
int
gpio_open(int gpio, int input)
{
    int fd;
    char path[64];

    sprintf(path,GPIO_VALUE_PATH,gpio);

    if(input)
      fd = open(path,O_RDONLY);
    else
      fd = open(path,O_WRONLY);

    if(fd == -1)
    {
        err_output("Exception opening %s will retry\n", path);
        fd = gpio_check_direction_permissions_bug(path);
        if(input)
          fd = open(path,O_RDONLY);
        else
          fd = open(path,O_WRONLY);
        if(fd == -1)
          err_output("Exception opening %s\n", path);
    }
    printf("gpio_open: %s input=%d fd=%d\n", path, input, fd);
    return fd;
}

// close file /sys/class/gpio/gpio4/value
int
gpio_close(int fd)
{
    int ret;
    ret = close(fd);
    printf("gpio_close: fd=%d\n", fd);
    if(fd == -1)
    {
        err_output("Exception closing %d\n", fd);
    }
    return ret;
}

int
gpio_write(int fd, int val)
{
    char cval[4];
    sprintf(cval,"%d",val);
    int ret = write(fd,cval,strlen(cval));
    if(ret<0)
        perror("GPIO: unable to perform gpio write");
    return ret;
}

int
gpio_read(int fd, char *val)
{
    char cval[2];
    int ret;
    ret = read(fd, cval, 2);
    if(ret!=2)
        perror("GPIO: unable to perform gpio read");
    *val = cval[0]-48;
    return ret;
}

// echo 4 > /sys/class/gpio/unexport
int
gpio_unexport(int gpio)
{
    int fd;
    char path[64];
    char val[3];

    sprintf(path,GPIO_UNEXPORT_PATH);
    sprintf(val,"%d",gpio);
    printf("gpio_unexport: %s %s\n",path,val);
    fd = open(path,O_WRONLY);
    if(fd == -1)
    {
      fprintf(stderr,"Exception unexporting GPIO %s %s",path, val);
      return -1;
    }

    if(write(fd,val,strlen(val))<0)
    {
      err_output("GPIO: Unable to unexport %s %s", path, val);
      return -1;
    }

    return close(fd);
}
