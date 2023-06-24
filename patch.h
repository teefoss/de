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

    // x, y used for texture palette location and are relative to
    // the texture panel's paletteRect.
    SDL_Rect rect;

    int numPatches;
    Patch patches[MAX_PATCHES];

    SDL_Texture * texture; // Loaded on the fly.
} Texture;

extern Array * resourceTextures;

void InitPlayPalette(const Wad * wad);
Patch LoadPatch(const Wad * wad, int lumpIndex);
void LoadAllPatches(const Wad * wad);
void LoadAllTextures(const Wad * wad);
void FreePatchesAndTextures(void);
Texture * FindTexture(const char * name);

void RenderTextureInRect(const char * name, const SDL_Rect * rect);
void RenderPatchInRect(const Patch * patch, const SDL_Rect * rect);
void RenderTexture(Texture * texture, int x, int y, float scale);

#endif /* patch_h */
