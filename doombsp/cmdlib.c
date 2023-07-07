// cmdlib.c

#include "cmdlib.h"

#ifndef __NeXT__

#define PATHSEPERATOR   '\\'
#endif

#ifdef __NeXT__

#define O_BINARY        0
#define PATHSEPERATOR   '/'



/*
================
=
= filelength
=
================
*/

int filelength (int handle)
{
	struct stat	fileinfo;
    
	if (fstat (handle,&fileinfo) == -1)
	{
		fprintf (stderr,"Error fstating");
		exit (1);
	}

	return fileinfo.st_size;
}

int tell (int handle)
{
	return lseek (handle, 0, L_INCR);
}

char *strupr (char *start)
{
	char	*in;
	in = start;
	while (*in)
	{
		*in = toupper(*in);
		in++;
	}
	return start;
}

char *getcwd (char *path, int length)
{
	return getwd(path);
}

#endif


/*
=================
=
= Error
=
= For abnormal program terminations
=
=================
*/

void Error (char *error, ...)
{
	va_list argptr;

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	exit (1);
}
