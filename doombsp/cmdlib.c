// cmdlib.c

#include "cmdlib.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void Error(char *error, ...)
{
	va_list argptr;

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	exit (1);
}
