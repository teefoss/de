//
//  common.c
//  de
//
//  Created by Thomas Foster on 5/20/23.
//

#include "common.h"

void _assert(const char * message, const char * file, int line)
{
    fflush(NULL);
    fprintf(stderr, "%s:%d Assertion '%s' Failed.\n", file, line, message);
    fflush(stderr);
}
