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

#define MAP_LABEL_LENGTH 6

typedef struct {
    s16 x;
    s16 y;
} Point;

typedef struct {
    s16 v1;
    s16 v2;
    s16 flags;
    s16 special;
    s16 tag;
} Line;

typedef struct {
    s16 x;
    s16 y;
    s16 angle;
    s16 type;
    s16 options;
} Thing;

typedef struct {
    char label[MAP_LABEL_LENGTH]; // "ExMx" or "MAPxx" + \0
    Array * points;
    Array * lines;
    Array * things;
} Map;

extern Map map;

void LoadMap(const Wad * wad, const char * lumpLabel);
SDL_Rect GetMapBounds(void);
int Translate(int y);

#endif /* Map_h */
