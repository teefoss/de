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
Array * resourceTextures;

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
            printf("loaded patch %s\n", patch.name);
            Push(patches, &patch);
        }

        CloseProgressPanel();
        ++section;
    }

    printf("loaded %d patches: %d ms\n", count, SDL_GetTicks() - startMS);
}

void RenderPatch(const Patch * patch, int x, int y, float scale)
{
    SDL_Rect dest = { x, y, patch->rect.w * scale, patch->rect.h * scale };
    SDL_RenderCopy(renderer, patch->texture, NULL, &dest);
}

void LoadAllTextures(const Wad * wad)
{
    char * pnames = GetLumpWithName(wad, "PNAMES") + 4;

    resourceTextures = NewArray(0, sizeof(Texture), 1);

    int section = 1;

    while ( 1 )
    {
        char lumpLabel[10] = { 0 };
        snprintf(lumpLabel, sizeof(lumpLabel), "TEXTURE%d", section);
        void * data = GetLumpWithName(wad, lumpLabel);

        if ( data == NULL )
        {
            if ( section == 1 )
            {
                fprintf(stderr, "This WAD has no textures!\n");
                return;
            }

            break;
        }

        u32 numTextures = *(u32 *)data;
        u32 * offsets = (u32 *)(data + 4);

        for ( int i = 0; i < numTextures; i++ )
        {
            maptexture_t * mtexture = (maptexture_t *)(data + offsets[i]);
            Texture texture;
            strncpy(texture.name, mtexture->name, 8);
            texture.name[8] = '\0';
            texture.width = mtexture->width;
            texture.height = mtexture->height;
            texture.numPatches = mtexture->patchcount;
            texture.texture = NULL;

            mappatch_t * texturePatches = mtexture->patches;
            Patch * allPatches = patches->data;

            for ( int j = 0; j < texture.numPatches; j++ )
            {
                mappatch_t patch = texturePatches[j];
                char * name = &pnames[patch.patch * 8];

                for ( int k = 0; k < patches->count; k++ )
                {
                    if ( strncmp(name, allPatches[k].name, 8) == 0 )
                    {
                        texture.patches[j] = allPatches[k];
                        break;
                    }
                }

                texture.patches[j].rect.x = patch.originx;
                texture.patches[j].rect.y = patch.originy;
            }

            Push(resourceTextures, &texture);

            printf("%s: %d x %d\n", texture.name, texture.width, texture.height);
            printf("  patches (%d):\n", texture.numPatches);
            for ( int j = 0; j < texture.numPatches; j++ )
            {
                printf("  - %s: %d, %d\n",
                       texture.patches[j].name,
                       texture.patches[j].rect.x,
                       texture.patches[j].rect.y);
            }
        }

        section++;
    }
}

Texture * FindTexture(const char * name)
{
    Texture * texture = resourceTextures->data;

    for ( int i = 0; i < resourceTextures->count; i++, texture++ )
        if ( strcmp(texture->name, name) == 0 )
            return texture;

    return NULL;
}

void RenderTexture(const Texture * texture, int x, int y, float scale)
{
    for ( int j = 0; j < texture->numPatches; j++ )
        RenderPatch(&texture->patches[j],
                    x + texture->patches[j].rect.x * scale,
                    y + texture->patches[j].rect.y * scale,
                    scale);
}

void RenderTextureInRect(const char * name, const SDL_Rect * rect)
{
    Texture * texture = FindTexture(name);

    if ( texture->texture == NULL )
    {
        texture->texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET,
                                             texture->width,
                                             texture->height);

        if ( texture->texture == NULL )
        {
            printf("Could not create texture for %s!\n", name);
        }

        SDL_SetRenderTarget(renderer, texture->texture);
        RenderTexture(texture, 0, 0, 1.0f);
        SDL_SetRenderTarget(renderer, NULL);
    }

    float scale;
    int xOffset = 0;
    int yOffset = 0;
    if ( texture->width > texture->height ) {
        scale = (float)rect->w / texture->width;
        yOffset = (rect->h - texture->height * scale) / 2.0f;
    } else {
        scale = (float)rect->h / texture->height;
        xOffset = (rect->w - texture->width * scale) / 2.0f;
    }

    SDL_Rect dst = {
        .x = rect->x + xOffset, // center in rect
        .y = rect->y + yOffset,
        .w = texture->width * scale,
        .h = texture->height * scale
    };

    SDL_RenderCopy(renderer, texture->texture, NULL, &dst);
//    RenderTexture(texture, rect->x, rect->y, scale);
}
