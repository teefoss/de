//
//  flat.c
//  de
//
//  Created by Thomas Foster on 6/24/23.
//

#include "flat.h"
#include "patch.h"

/* Flat */ Array * flats;

void LoadFlat(Wad * wad, int lumpIndex, SDL_Color * playpal)
{
    Flat flat = { 0 };
    flat.rect.w = 64;
    flat.rect.h = 64;
    flat.texture = SDL_CreateTexture(renderer,
                                     SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_TARGET,
                                     64,
                                     64);

    if ( flat.texture == NULL )
        fprintf(stderr, "Could not create flat texture! (%s)\n", SDL_GetError());

    const char * name = GetLumpName(wad, lumpIndex);
    strncpy(flat.name, name, 8);

    u8 * flatData = GetLumpWithIndex(wad, lumpIndex);

    SDL_SetRenderTarget(renderer, flat.texture);

    for ( int y = 0; y < 64; y++ )
    {
        for ( int x = 0; x < 64; x++ )
        {
            SetRenderDrawColor(&playpal[flatData[y * 64 + x]]);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_SetRenderTarget(renderer, NULL);

    Push(flats, &flat);
}

void LoadFlats(Wad * wad)
{
    int set = 0;
    flats = NewArray(0, sizeof(Flat), 1);

    SDL_Color playpal[256];
    GetPlayPalette(wad, playpal);

    while ( 1 )
    {
        char startLabel[10];
        char endLabel[10];
        sprintf(startLabel, "F%d_START", set + 1);
        sprintf(endLabel, "F%d_END", set + 1);

        int startIndex = GetLumpIndex(wad, startLabel);
        int endIndex = GetLumpIndex(wad, endLabel);

        if ( startIndex == -1 || endIndex == -1 )
        {
            if ( set == 0 )
                fprintf(stderr, "Could not find any flats in this WAD!\n");
            return;
        }

        printf("Loading flat set %d...\n", set + 1);

        for ( int i = startIndex + 1; i < endIndex; i++ )
            LoadFlat(wad, i, playpal);

        set++;
    }
}

Flat * GetFlat(const char * name)
{
    Flat * flat = flats->data;

    for ( int i = 0; i < flats->count; i++, flat++ )
        if ( strcmp(flat->name, name) == 0 )
            return flat;

    return NULL;
}

void RenderFlat(const char * name, int x, int y, float scale)
{
    Flat * flat = GetFlat(name);

    if ( flat == NULL )
    {
        fprintf(stderr, "Try to render flat with wonkey name (%s)!\n", name);
        return;
    }

    SDL_Rect dest = { x, y, 64 * scale, 64 * scale };
    SDL_RenderCopy(renderer, flat->texture, NULL, &dest);
}
