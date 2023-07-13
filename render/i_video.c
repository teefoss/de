//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

#include <SDL2/SDL.h>

#include "v_video.h"
#include "doomdef.h"

#include "e_editor.h"
#include "wad.h"


static SDL_Window * viewWindow;
static SDL_Renderer * viewRenderer;
SDL_Texture * screen;
static SDL_Color colors[256];

void I_ShutdownGraphics(void)
{
    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(viewRenderer);
    SDL_DestroyWindow(viewWindow);
}



//
// I_StartTic
//
void I_StartTic (void)
{

}


//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    SDL_Surface * surface = SDL_CreateRGBSurface(0, SCREENWIDTH, SCREENHEIGHT, 8, 0, 0, 0, 0);
    SDL_SetPaletteColors(surface->format->palette, colors, 0, 256);

//    SDL_RenderClear(viewRenderer);
//    SDL_SetRenderTarget(viewRenderer, screen);
//
    SDL_LockSurface(surface);
    for ( int y = 0; y < SCREENHEIGHT; y++ )
    {
        for ( int x = 0; x < SCREENWIDTH; x++ )
        {
            u8 * pixel = surface->pixels;
            pixel[y * surface->pitch + x] = *(screens[0] + y * SCREENWIDTH + x);
//            u8 * pixel = screens[0] + y * SCREENWIDTH + x;
//            SDL_Color c = colors[*pixel];
//            SDL_SetRenderDrawColor(viewRenderer, c.r, c.g, c.b, 255);
//            SDL_RenderDrawPoint(viewRenderer, x, y);
        }
    }
    SDL_UnlockSurface(surface);

    SDL_Texture * texture = SDL_CreateTextureFromSurface(viewRenderer, surface);

//
//    SDL_SetRenderTarget(viewRenderer, NULL);
    SDL_RenderCopy(viewRenderer, texture, NULL, NULL);
    SDL_RenderPresent(viewRenderer);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    for ( int i = 0; i < 256; i++ )
    {
        colors[i].r = gammatable[usegamma][*palette++] & ~3;
        colors[i].g = gammatable[usegamma][*palette++] & ~3;
        colors[i].b = gammatable[usegamma][*palette++] & ~3;

        printf("loaded palette %d: %d, %d, %d\n", i, colors[i].r, colors[i].g, colors[i].b);
    }
}



void I_InitGraphics(void)
{
    u32 windowFlags = 0;
    viewWindow = SDL_CreateWindow("Live View", 0, 0, 320, 240, windowFlags);
    u32 pixelFormat = SDL_GetWindowPixelFormat(viewWindow);

    u32 rendererFlags = 0;
    viewRenderer = SDL_CreateRenderer(viewWindow, -1, rendererFlags);
    SDL_RenderSetLogicalSize(viewRenderer, 320, 240);

    Lump * paletteLump = GetLumpNamed(editor.iwad, "PLAYPAL");
    I_SetPalette(paletteLump->data);

    screen = SDL_CreateTexture(viewRenderer,
                               pixelFormat,
                               SDL_TEXTUREACCESS_TARGET,
                               SCREENWIDTH, SCREENHEIGHT);

    atexit(I_ShutdownGraphics);
}
