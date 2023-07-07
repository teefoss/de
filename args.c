//
//  args.c
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#include "args.h"
#include <string.h>

static int     _argc;
static char ** _argv;

void InitArgs(int argc, char ** argv)
{
    _argc = argc;
    _argv = argv;
}

int GetArg(const char * string)
{
    size_t len = strlen(string);

    for ( int i = 0; i < _argc; i++ ) {
        if ( strncmp(string, _argv[i], len) == 0 ) {
            return i;
        }
    }

    return -1;
}

int GetArg2(const char * string, const char * alternative)
{
    int index = GetArg(string);
    if ( index == -1 )
        index = GetArg(alternative);

    return index;
}

char * GetOptionArg(const char * option)
{
    int index = GetArg(option);
    if ( index == -1 || index + 1 >= _argc ) {
        return NULL;
    }

    return _argv[index + 1];
}

char * GetOptionArg2(const char * option, const char * alternative)
{
    char * arg = GetOptionArg(option);
    if ( arg == NULL )
        arg = GetOptionArg(alternative);

    return arg;
}
