#ifndef _TOUCH_H
#define _TOUCH_H

#include <stdio.h>		
#include <sys/types.h>		
#include <sys/stat.h>		
#include <sys/mman.h>		
#include <fcntl.h>		
#include <unistd.h>		
#include <string.h>		
#include <stdbool.h>		
#include <linux/input.h>

int wait4touch(int *x, int *y);
int wait4leave(int *x, int *y);


#endif