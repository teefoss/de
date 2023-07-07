#ifndef __CMDLIB__
#define __CMDLIB__

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef unsigned char byte;
#endif

/// For abnormal program terminations
void Error(char *error, ...);

#endif
