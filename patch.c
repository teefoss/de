//
//  patch.c
//  de
//
//  Created by Thomas Foster on 6/11/23.
//

#include "patch.h"
#include "doomdata.h"
#include "wad.h"
#include "array.h"
#include "progress_panel.h"
#include "edit.h"

SDL_Color playPalette[256];
Array * patches;

void InitPlayPalette(const Wad * wad)
{
    u8 * palLBM = GetLumpWithName(wad, "playpal");

    if ( palLBM == NULL )
    {
        fprintf(stderr, "WAD file has no playpal!\n");
        return;
    }

    u8 * component = palLBM;

    for ( int i = 0; i < 256; i++ )
    {
        playPalette[i].r = *component++;
        playPalette[i].g = *component++;
        playPalette[i].b = *component++;
        playPalette[i].a = 255;
    }

    free(palLBM);
}

Patch LoadPatch(const Wad * wad, int lumpIndex)
{
    patch_t * patchData = GetLumpWithIndex(wad, lumpIndex);
    const char * name = GetLumpName(wad, lumpIndex);

    Patch patch;
    patch.rect.w = patchData->width;
    patch.rect.h = patchData->height;
    strncpy(patch.name, name, 8);
    patch.name[8] = '\0';

    patch.texture = SDL_CreateTexture(renderer,
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET,
                                      patch.rect.w,
                                      patch.rect.h);

    if ( patch.texture == NULL )
    {
        fprintf(stderr, "Could not create texture for patch '%s'!\n", patch.name);
        free(patchData);
        return patch;
    }

    SDL_SetTextureBlendMode(patch.texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, patch.texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for ( int x = 0; x < patch.rect.w; x++ )
    {
        const u8 * data = (u8 *)patchData + patchData->columnofs[x];

        while ( 1 )
        {
            int y = *data++;
            if ( y == (u8)-1 )
                break;

            int count = *data++;
            data++; // Skip unused byte.

            while ( count-- )
            {
                SDL_Color color = playPalette[*data++];
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
                SDL_RenderDrawPoint(renderer, x, y++);
            }

            data++; // Skip unused byte.
        }
    }

    SDL_SetRenderTarget(renderer, NULL);

    free(patchData);

    return patch;
}

void LoadAllPatches(const Wad * wad)
{
    int startMS = SDL_GetTicks();

    patches = NewArray(0, sizeof(Patch), 1);

    int section = 1;
    int count = 0;

    while ( 1 )
    {
        char startLabel[10];
        char endLabel[10];
        snprintf(startLabel, 10, "P%d_START", section);
        snprintf(endLabel, 10, "P%d_END", section);

        int start = GetLumpIndex(wad, startLabel);
        int end = GetLumpIndex(wad, endLabel);

        if ( start == -1 || end == -1 )
        {
            if ( section == 1 )
            {
                fprintf(stderr, "No patches found in this WAD!");
                return;
            }

            break; // Finished
        }

        for ( int i = start + 1; i < end; i++ )
        {
            Patch patch = LoadPatch(wad, i);
            Push(patches, &patch);
        }

        CloseProgressPanel();
        ++section;
    }

    printf("loaded %d patches: %d ms\n", count, SDL_GetTicks() - startMS);
}

void RenderPatch(const Patch * patch, int x, int y, int scale)
{
    SDL_Rect dest = { x, y, patch->rect.w * scale, patch->rect.h * scale };
    SDL_RenderCopy(renderer, patch->texture, NULL, &dest);
}
