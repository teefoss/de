// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
//typedef enum {false, true} boolean;
typedef bool boolean;
typedef unsigned char byte;
#endif

void	Error (char *error, ...);

#endif
