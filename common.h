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

#define CLAMP(a, min, max) if (a<min) { a=min; } else if (a>max) { a=max; }
#define SIGN(a) (a < 0 ? -1 : a > 0 ? 1 : 0)
#define MAX(a, b) ((a > b) ? (a) : (b))
#define MIN(a, b) ((a < b) ? (a) : (b))

#define SWAP16(x)   SDL_SwapLE16(x)
#define SWAP32(x)   SDL_SwapLE32(x)

#define STRNEQ(a, b, n) (strncmp(a, b, n) == 0)

#if DEBUG
    #define ASSERT(expr) \
        void _assert(const char * message, const char * file, int line); \
        if ( expr ) { } \
        else { \
            _assert(#expr, __FILE__, __LINE__); \
            abort(); \
        }
#else
    #define ASSERT(expr)
#endif

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

extern SDL_Window * window;
extern SDL_Renderer * renderer;

// TODO: organize this!
void InitWindow(int width, int height);
int GetRefreshRate(void);
float LerpEpsilon(float a, float b, float w, float epsilon);
SDL_Texture * GetScreen(void);
void draw_line_antialias(int x1, int y1, int x2, int y2);
void Capitalize(char * string);
void DrawRect(const SDL_FRect * rect, int thinkness);
void SetRenderDrawColor(const SDL_Color * c);
SDL_Color Int24ToSDL(int integer);
SDL_Rect GetWindowFrame(void);
void GuptaSprollDrawLine(float x1, float x2, float y1, float y2);
//double Map(double input,
//           double input_start,
//           double input_end,
//           double output_start,
//           double output_end);
void WuDrawLine(double x0, double y0, double x1, double y1);
void FosterDrawLineAA(int x1, int y1, int x2, int y2);
void Refresh(SDL_Renderer * _renderer, SDL_Texture * _texture);

#endif /* defines_h */
