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
#include <stdbool.h>

#define MAP_LABEL_LENGTH 6

typedef struct {
    SDL_Point origin;
    bool selected;
    int referenceCount;
    bool removed;
} Vertex;

typedef struct {
    s16 flags;
    s16 sector;
//    s16
} Side;

typedef struct {
    s16 v1;
    s16 v2;
    s16 flags;
    s16 special;
    s16 tag;

    bool selected;

    // Which side should be shown when opening the line panel.
    bool panelBackSelected;
} Line;

typedef struct {
    SDL_Point origin;
    s16 angle;
    s16 type;
    s16 options;

    bool selected;
} Thing;

typedef struct {
    char label[MAP_LABEL_LENGTH]; // "ExMx" or "MAPxx" + \0
    Array * vertices;
    Array * lines;
    Array * things;
} Map;

extern Map map;

void LoadMap(const Wad * wad, const char * lumpLabel);
SDL_Rect GetMapBounds(void);
SDL_Point LineMidpoint(const Line * line);
float LineLength(const Line * line);

#endif /* Map_h */
