//
//  line.h
//  de
//
//  Created by Thomas Foster on 6/27/23.
//

#ifndef line_h
#define line_h

#include "common.h"
#include "next.h"
#include <stdbool.h>

typedef enum
{
    VISIBILITY_NONE,
    VISIBILITY_PARTIAL,
    VISIBILITY_FULL
} Visibility;

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
    SectorDef sectorDef;
    int sectorNum; // Used by the node builder.
} Side;

typedef struct
{
    NXPoint p1, p2; // Only used by node builder.

    int v1; // The index in map.vertices
    int v2; // The index in map.vertices

    int flags;
    int special;
    int tag;

    Side sides[2];

    enum
    {
        DESELECTED,
        FRONT_SELECTED,
        BACK_SELECTED,
    } selected;

    bool deleted;

    // Which side should be shown when opening the line panel.
    bool panelBackSelected;
} Line;

void        InitLineCross(void);
SDL_FPoint  LineMidpoint(const Line * line);
float       LineLength(const Line * line);
SDL_FPoint  LineNormal(const Line * line, float length);
void        GetLinePoints(int index, SDL_Point * p1, SDL_Point * p2);
Side *      SelectedSide(Line * line);
void        ClipLine(int lineIndex, SDL_Point * out1, SDL_Point * out2);
Visibility  LineVisibility(int index);

#endif /* line_h */
