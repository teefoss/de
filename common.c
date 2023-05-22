//
//  common.c
//  de
//
//  Created by Thomas Foster on 5/20/23.
//

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#include "common.h"

void _assert(const char * message, const char * file, int line)
{
    fflush(NULL);
    fprintf(stderr, "%s:%d Assertion '%s' Failed.\n", file, line, message);
    fflush(stderr);
}



SDL_Window * window;
SDL_Renderer * renderer;

static void CleanUpWindowAndRenderer(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void InitWindow(void)
{
    atexit(CleanUpWindowAndRenderer);

    if ( SDL_Init(SDL_INIT_VIDEO) != 0 )
    {
        fprintf(stderr, "Error: could not create window (%s)\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;
    u32 flags = 0;
    flags |= SDL_WINDOW_RESIZABLE;
    flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    window = SDL_CreateWindow("", x, y, WINDOW_WIDTH, WINDOW_HEIGHT, flags);

    if ( window == NULL )
    {
        fprintf(stderr, "Error: could not create window (%s)\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    if ( renderer == NULL )
    {
        fprintf(stderr,
                "Error: could not create renderer (%s)\n",
                SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // Adjust for High DPI displays.
    SDL_Rect renderSize;
    SDL_GetRendererOutputSize(renderer, &renderSize.w, &renderSize.h);
    if ( renderSize.w != WINDOW_WIDTH && renderSize.h != WINDOW_HEIGHT ) {
        SDL_RenderSetScale(renderer,
                           renderSize.w / WINDOW_WIDTH,
                           renderSize.h / WINDOW_HEIGHT);
    }
}

int GetRefreshRate(void)
{
    int displayIndex = SDL_GetWindowDisplayIndex(window);

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(displayIndex, &displayMode);

    return displayMode.refresh_rate;
}
