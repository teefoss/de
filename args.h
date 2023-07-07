//
//  args.h
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#ifndef args_h
#define args_h

void InitArgs(int argc, char ** argv);

/// Get index of argument, `string`. Returns -1 on fail.
int GetArg(const char * string);

int GetArg2(const char * string, const char * alternative);

/// Returns the argument one after `option` or `NULL` if fail.
char * GetOptionArg(const char * option);

/// For an option with two formats, e.g. -m and --map, return argument
/// following the option, or `NULL` on fail.
char * GetOptionArg2(const char * option, const char * alternative);

#endif /* args_h */
