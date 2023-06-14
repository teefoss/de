//
//  patch.h
//  de
//
//  Created by Thomas Foster on 6/11/23.
//

#ifndef patch_h
#define patch_h

#include "wad.h"
#include <SDL2/SDL.h>
#include <stdbool.h>

#define MAX_PATCHES 100 // Max patches per texture

typedef struct
{
    SDL_Rect rect;
    char name[9];
    SDL_Texture * texture;
} Patch; // A loaded patch for use in the editor

typedef struct
{
    char name[9];
    int width;
    int height;
    int numPatches;
    Patch patches[MAX_PATCHES];
    SDL_Texture * texture; // Loaded on the fly.
} Texture;

void InitPlayPalette(const Wad * wad);
Patch LoadPatch(const Wad * wad, int lumpIndex);
void LoadAllPatches(const Wad * wad);
void LoadAllTextures(const Wad * wad);

void RenderTextureInRect(const char * name, const SDL_Rect * rect);

#endif /* patch_h */
