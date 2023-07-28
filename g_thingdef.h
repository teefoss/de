//
//  g_thingdef.h
//  de
//
//  Created by Thomas Foster on 7/27/23.
//

#ifndef g_thingdef_h
#define g_thingdef_h

#include "e_editor.h"
#include "g_patch.h"

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

typedef struct
{
    ThingCategory category;
    int doomedType;
    Game game;
    int radius;
    Patch patch;
    char name[64];
} ThingDef;

typedef struct
{
    int startIndex; // in thingDefs array
    int count;
    float paletteScale;
} ThingCategoryInfo;


extern ThingDef * thingDefs;
extern ThingCategoryInfo categoryInfo[THING_CATEGORY_COUNT];

void LoadThingDefinitions(void);
ThingDef * GetThingDef(int type);

#endif /* g_thingdef_h */
