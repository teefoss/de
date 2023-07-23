//
//  common.c
//  de
//
//  Created by Thomas Foster on 5/20/23.
//

#include "common.h"
#include "e_defaults.h"
#include <stdbool.h>
#include <stdarg.h>

void _assert(const char * message, const char * file, int line)
{
    fflush(NULL);
    fprintf(stderr, "%s:%d Assertion '%s' Failed.\n", file, line, message);
    fflush(stderr);
}


void Error(const char * format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf ("\n");

    exit(EXIT_FAILURE);
}

SDL_Window * window;
SDL_Renderer * renderer;

static void CleanUpWindowAndRenderer(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void InitWindow(int width, int height)
{
    atexit(CleanUpWindowAndRenderer);

    if ( SDL_Init(SDL_INIT_VIDEO) != 0 )
    {
        fprintf(stderr, "Error: could not init SDL (%s)\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;
    u32 flags = 0;
    flags |= SDL_WINDOW_RESIZABLE;
//    flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    window = SDL_CreateWindow("", x, y, width, height, flags);

    if ( window == NULL )
    {
        fprintf(stderr, "Error: could not create window (%s)\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    u32 rendererFlags = 0;
//    rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
    rendererFlags |= SDL_RENDERER_TARGETTEXTURE;
    rendererFlags |= SDL_RENDERER_SOFTWARE;
//    rendererFlags |= SDL_RENDERER_ACCELERATED;
    renderer = SDL_CreateRenderer(window, -1, rendererFlags);

    if ( renderer == NULL )
    {
        fprintf(stderr,
                "Error: could not create renderer (%s)\n",
                SDL_GetError());
        exit(EXIT_FAILURE);
    }

//    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
//    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    // Adjust for High DPI displays.
    // TODO: nix this for now. Things get weird when the window resizes...
#if 0
    SDL_Rect renderSize;
    SDL_GetRendererOutputSize(renderer, &renderSize.w, &renderSize.h);
    if ( renderSize.w != WINDOW_WIDTH && renderSize.h != WINDOW_HEIGHT ) {
        SDL_RenderSetScale(renderer,
                           renderSize.w / WINDOW_WIDTH,
                           renderSize.h / WINDOW_HEIGHT);
    }
#endif
}

void SetRenderDrawColor(const SDL_Color * c)
{
    SDL_SetRenderDrawColor(renderer, c->r, c->g, c->b, c->a);
}

int GetRefreshRate(void)
{
    int displayIndex = SDL_GetWindowDisplayIndex(window);

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(displayIndex, &displayMode);

    return displayMode.refresh_rate;
}

float Lerp(float a, float b, float w)
{
    return (1.0f - w) * a + w * b;
}

float LerpEpsilon(float a, float b, float w, float epsilon)
{
    float result = (1.0f - w) * a + w * b;
    if ( fabsf(result - b) < epsilon ) {
        result = b;
    }

    return result;
}

SDL_Texture * GetScreen(void)
{
    SDL_Surface * surface = SDL_GetWindowSurface(window);
    if ( surface == NULL )
        printf("%s\n", SDL_GetError());
    return SDL_CreateTextureFromSurface(renderer, surface);
}



void Capitalize(char * string)
{
    if ( string == NULL )
        return;

    for ( char * c = string; *c; c++ )
        *c = toupper(*c);
}

void DrawRect(const SDL_FRect * rect, int thinkness)
{
    SDL_FRect r = *rect;

    for ( int i = 0; i < thinkness; i++ )
    {
        SDL_RenderDrawRectF(renderer, &r);
        r.x++;
        r.y++;
        r.w -= 2.0f;
        r.h -= 2.0f;

        if ( r.w <= 0.0f || r.h <= 0.0f )
            break;
    }
}

SDL_Color Int24ToSDL(int integer)
{
    SDL_Color color;

    color.r = (integer & 0xFF0000) >> 16;
    color.g = (integer & 0x00FF00) >> 8;
    color.b = integer & 0x0000FF;
    color.a = 255;

    return color;
}

SDL_Rect GetWindowFrame(void)
{
    SDL_Rect frame;
    SDL_GetWindowPosition(window, &frame.x, &frame.y);
    SDL_GetWindowSize(window, &frame.w, &frame.h);

    return frame;
}

void Bresenham(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;

    int error = dx + dy;
    int error2;

    int x = x1;
    int y = y1;

    while ( x != x2 || y != y2 )
    {
        SDL_RenderDrawPoint(renderer, x, y);

        error2 = error * 2;

        if ( error2 >= dy )
        {
            error += dy;
            x += sx;
        }

        if ( error2 <= dx )
        {
            error += dx;
            y += sy;
        }
    }
}

void Refresh(SDL_Renderer * _renderer, SDL_Texture * _texture)
{
    SDL_SetRenderTarget(_renderer, NULL);
    SDL_RenderClear(_renderer);
    SDL_RenderCopy(_renderer, _texture, NULL, NULL);
    SDL_RenderPresent(_renderer);
    SDL_SetRenderTarget(_renderer, _texture);
    SDL_PumpEvents();
}
