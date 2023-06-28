//
//  Map.h
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#ifndef Map_h
#define Map_h

#include "array.h"
#include "wad.h"
#include "vertex.h"
#include "line.h"
#include "thing.h"

#include <stdbool.h>

#define MAP_LABEL_LENGTH 6

typedef struct
{
    char label[MAP_LABEL_LENGTH]; // "ExMx" or "MAPxx" + \0
    Array * vertices;
    Array * lines;
    Array * things;

    SDL_Rect bounds;
    bool boundsDirty;
} Map;

extern Map map;

void LoadMap(const Wad * wad, const char * lumpLabel);
SDL_Rect GetMapBounds(void);

#endif /* Map_h */
