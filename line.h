//
//  line.h
//  de
//
//  Created by Thomas Foster on 6/27/23.
//

#ifndef line_h
#define line_h

#include "common.h"
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

void        InitLineCross(void);
SDL_Point   LineMidpoint(const Line * line);
float       LineLength(const Line * line);
SDL_Point   LineNormal(const Line * line, float length);
void        GetLinePoints(int index, SDL_Point * p1, SDL_Point * p2);
Side *      SelectedSide(Line * line);
void        ClipLine(int lineIndex, SDL_Point * out1, SDL_Point * out2);
Visibility  LineVisibility(int index);

#endif /* line_h */
