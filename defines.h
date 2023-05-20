//
//  defines.h
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#ifndef defines_h
#define defines_h

#include <SDL2/SDL.h>
#include <stdint.h>
#include <string.h>

#define SWAP16(x)   SDL_SwapLE16(x)
#define SWAP32(x)   SDL_SwapLE32(x)
#define STRNEQ(a, b, n) (strncmp(a, b, n) == 0)

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

#endif /* defines_h */
