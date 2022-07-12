#ifndef _MYSQLITE_H
#define _MYSQLITE_H

#include <stdio.h>
#include <assert.h>
#include <fcntl.h> 
#include <unistd.h>
//#include <termios.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <strings.h> // for bzero()
#include <stdbool.h>

#include "sqlite3.h"
int showDB(void *arg, int len, char **col_val, char **col_name);
void *carIn(void *arg);
void *carOut(void *arg)
void *fun_sqlite(void *arg);


#endif