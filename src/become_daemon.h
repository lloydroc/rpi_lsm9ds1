#ifndef BECOME_DAEMON_H
#define BECOME_DAEMON_H
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// returns 0 on success -1 on error
int 
become_daemon();

int
write_pidfile(char *file);

#endif
