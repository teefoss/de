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

/// Create a new, blank map.
void CreateMap(const char * label);

/// Load map from WAD lump.
void LoadMap(const Wad * wad, const char * lumpLabel);

/// Load map from .dwd file.
void LoadDWD(const char * mapName);

/// Save the currently loaded map to .dwd
void SaveDWD(void);

SDL_Rect GetMapBounds(void);
void TranslateCoord(int * y, const SDL_Rect * bounds);
Line * NewLine(const SDL_Point * p1, const SDL_Point * p2);
bool GetClosestSide(const SDL_Point * point, Side * out);

#endif /* Map_h */
