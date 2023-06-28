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
#include "thing.h"

#include <stdbool.h>

#define MAP_LABEL_LENGTH 6

typedef enum
{
    CR_BEFORE,
    CR_INSIDE,
    CR_AFTER,
} ClippingRegion;

typedef enum
{
    VISIBILITY_NONE,
    VISIBILITY_PARTIAL,
    VISIBILITY_FULL
} Visibility;

typedef struct
{
    SDL_Point origin;
    bool selected;
    int referenceCount;
    bool removed;
} Vertex;

typedef struct
{
    int floorHeight;
    int ceilingHeight;
    char floorFlat[9];
    char ceilingFlat[9];
    int lightLevel;
    int special;
    int tag;
} SectorDef;

typedef struct
{
    int offsetX;
    int offsetY;
    char bottom[9];
    char middle[9];
    char top[9];
    SectorDef sector;
} Side;

typedef struct
{
    s16 v1;
    s16 v2;
    s16 flags;
    s16 special;
    s16 tag;
    Side sides[2];

    enum
    {
        DELETED = -1, // TODO: this should be separate
        DESELECTED,
        FRONT_SELECTED,
        BACK_SELECTED,
    } selected;

    // Which side should be shown when opening the line panel.
    bool panelBackSelected;
} Line;

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

void InitLineCross(void);
void LoadMap(const Wad * wad, const char * lumpLabel);
SDL_Rect GetMapBounds(void);
SDL_Point LineMidpoint(const Line * line);
float LineLength(const Line * line);
void GetLinePoints(int index, SDL_Point * p1, SDL_Point * p2);
Side * SelectedSide(Line * line);
void ClipLine(int lineIndex, SDL_Point * out1, SDL_Point * out2);
Visibility LineVisibility(int index);

#endif /* Map_h */
