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

static SDL_Surface * surface8;  // Palette surface
static SDL_Surface * surface32; // Same format as window (probably 32 bit?)
SDL_Texture * texture;

static SDL_Color colors[256];

void I_ShutdownGraphics(void)
{
    SDL_FreeSurface(surface8);
    SDL_FreeSurface(surface32);
    SDL_DestroyTexture(texture);
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
    SDL_LowerBlit(surface8, NULL, surface32, NULL);
    SDL_UpdateTexture(texture, NULL, surface32->pixels, surface32->pitch);

    SDL_RenderClear(viewRenderer);
    SDL_RenderCopy(viewRenderer, texture, NULL, NULL);
    SDL_RenderPresent(viewRenderer);
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

    // Paletted surface.

    surface8 = SDL_CreateRGBSurface(0, SCREENWIDTH, SCREENHEIGHT, 8, 0, 0, 0, 0);
    SDL_FillRect(surface8, NULL, 0);
    SDL_SetPaletteColors(surface8->format->palette, colors, 0, 256);

    // Surface in window format.

    u32 rmask, gmask, bmask, amask;
    int bpp;
    SDL_PixelFormatEnumToMasks(pixelFormat, &bpp, &rmask, &gmask, &bmask, &amask);
    surface32 = SDL_CreateRGBSurface(0,
                                     SCREENWIDTH, SCREENHEIGHT,
                                     bpp,
                                     rmask, gmask, bmask, amask);
    SDL_FillRect(surface32, NULL, 0);

    // Texture.

    texture = SDL_CreateTexture(viewRenderer,
                                pixelFormat,
                                SDL_TEXTUREACCESS_STREAMING,
                                SCREENWIDTH, SCREENHEIGHT);

    atexit(I_ShutdownGraphics);
}
