//
//  flat.h
//  de
//
//  Created by Thomas Foster on 6/24/23.
//

#ifndef flat_h
#define flat_h

#include <SDL2/SDL.h>
#include "wad.h"
#include "args.h"

typedef struct
{
    char name[9];
    SDL_Texture * texture;
    SDL_Rect rect; // location in Flat Palette
} Flat;

extern Array * flats;

void LoadFlats(Wad * wad);
void RenderFlat(const char * name, int x, int y, float scale);
void GetFlatName(int index, char * string);

#endif /* flat_h */
