//
//  window.c
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#include "window.h"

SDL_Window * window;
SDL_Renderer * renderer;

static void CleanUp(void)
{
    SDL_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void InitWindow(void)
{
    atexit(CleanUp);

    if ( SDL_Init(SDL_INIT_VIDEO) != 0 )
    {
        fprintf(stderr, "Error: could not create window (%s)\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int x = SDL_WINDOWPOS_CENTERED;
    int y = SDL_WINDOWPOS_CENTERED;
    window = SDL_CreateWindow("", x, y, 800, 800, SDL_WINDOW_RESIZABLE);

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
}

int GetRefreshRate(void)
{
    int displayIndex = SDL_GetWindowDisplayIndex(window);

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(displayIndex, &displayMode);

    return displayMode.refresh_rate;
}
