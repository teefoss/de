//
//  thing.h
//  de
//
//  Created by Thomas Foster on 6/18/23.
//

#ifndef thing_h
#define thing_h

#include "e_editor.h"
#include "patch.h"

typedef enum
{
    THING_CATEGORY_PLAYER,
    THING_CATEGORY_DEMON,
    THING_CATEGORY_WEAPON,
    THING_CATEGORY_PICKUP,
    THING_CATEGORY_DECOR,
    THING_CATEGORY_GORE,
    THING_CATEGORY_OTHER,

    THING_CATEGORY_COUNT,
} ThingCategory;

// Thing Definition
typedef struct
{
    ThingCategory category;
    int doomedType;
    Game game;
    int radius;
    Patch patch;
    char name[64];
} ThingDef;

typedef struct {
    SDL_Point origin;
    int angle;
    int type;
    int options;

    bool selected;
    bool deleted;
} Thing;

typedef struct
{
    int startIndex; // in thingDefs
    int count;
    int paletteScale;
} ThingCategoryInfo;

extern ThingDef * thingDefs;
extern ThingCategoryInfo categoryInfo[THING_CATEGORY_COUNT];

void LoadThingDefinitions(void);
ThingDef * GetThingDef(int type);

#endif /* thing_h */
