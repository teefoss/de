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

typedef struct
{
    SDL_Rect rect;
    char name[9];
    SDL_Texture * texture;
} Patch; // A loaded patch for use in the editor

void InitPlayPalette(const Wad * wad);
Patch LoadPatch(const Wad * wad, int lumpIndex);
void LoadAllPatches(const Wad * wad);
void RenderPatch(const Patch * patch, int x, int y, int scale);

#endif /* patch_h */
