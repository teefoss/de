//
//  g_thingdef.c
//  de
//
//  Created by Thomas Foster on 6/18/23.
//

#include "g_thingdef.h"
#include "wad.h"

#include <stdio.h>
#include <stdlib.h>

int totalNumThings; // Number of things in .dsp
int numThings; // Actual number parsed (< totalNumThings if DOOM 1)
ThingDef * thingDefs;
ThingCategoryInfo categoryInfo[THING_CATEGORY_COUNT];

const char * categoryNames[THING_CATEGORY_COUNT] =
{
    "Player",
    "Demon",
    "Weapon",
    "Pickup",
    "Decor",
    "Gore",
    "Other",
};

static void FreeThingDefs(void)
{
    free(thingDefs);
}

void CompareOldThingDefs(void)
{
#if 0
    const char * paths[3] = {
        "doom1/things.dsp",
        "doomSE/things.dsp",
        "doom2/things.dsp",
    };

    for ( int pathIndex = 0; pathIndex < 3; pathIndex++ )
    {
        const char * path = paths[pathIndex];

        FILE * file = fopen(path, "r");

        int oldNumThings = 0;
        fscanf(file, "numthings: %d\n", &oldNumThings);

        for ( int i = 0; i < oldNumThings; i++ )
        {
            int type = 0;
            char name[100] = { 0 };
            fscanf(file, "%s = %*d %d %*d (%*f %*f %*f) %*s", name, &type);

            for ( int newDefIndex = 0; newDefIndex < numThings; newDefIndex++ )
            {
                if ( type == thingDefs[newDefIndex].doomedType )
                    goto nextOldThing;
            }

            printf("New thing defs is missing type %d! ('%s', in file %s)\n",
                   type, name, path);

        nextOldThing:
            ;
        }

        fclose(file);
    }
#endif
}

// Removed: Gore 23 1 16 SKULH0 Dead_Lost_Soul

void LoadThingDefinitions(void)
{
    FILE * file = fopen("things.dsp", "r");
    if ( file == NULL )
    {
        fprintf(stderr, "Could not find things.dsp!\n");
        return;
    }

    fscanf(file, "numthings: %d\n", &totalNumThings);
    thingDefs = calloc(totalNumThings, sizeof(*thingDefs));
    int maxLen = 0;

    ThingCategory parseCategory = THING_CATEGORY_PLAYER;

    float large = 1.5;

    categoryInfo[THING_CATEGORY_PLAYER].paletteScale = large;
    categoryInfo[THING_CATEGORY_DEMON].paletteScale = 1.0f;
    categoryInfo[THING_CATEGORY_WEAPON].paletteScale = large;
    categoryInfo[THING_CATEGORY_PICKUP].paletteScale = large;
    categoryInfo[THING_CATEGORY_DECOR].paletteScale = 1.0f;
    categoryInfo[THING_CATEGORY_GORE].paletteScale = 1.0f;
    categoryInfo[THING_CATEGORY_OTHER].paletteScale = large;

    int margin = 16;
    int x = margin;
    int y = margin;
    int maxRowHeight = 0;

    extern SDL_Rect thingPaletteRectOffsets; // TODO: we need a big refactor!
    int paletteWidth = thingPaletteRectOffsets.w;
    numThings = 0;

    for ( int i = 0; i < totalNumThings; i++ )
    {
        ThingDef * def = &thingDefs[numThings];
        char categoryName[32] = { 0 };
        char sprite[9] = { 0 };

        if ( fscanf(file, "%s %d %d %d %s %s\n",
                    categoryName,
                    &def->doomedType,
                    &def->game,
                    &def->radius,
                    sprite,
                    def->name) != 6 )
        {
            break;
        }

        if ( def->game > editor.game )
            continue;

        // Set category number.
        bool found = false;
        for ( int j = 0; j < THING_CATEGORY_COUNT; j++ )
        {
            if ( strcmp(categoryNames[j], categoryName) == 0 ) {
                def->category = j;
                found = true;
                break;
            }
        }

        if ( !found )
            fprintf(stderr, "Bad category name! (%s)\n", categoryName);

        // Reading a new category?
        if ( def->category > parseCategory )
        {
            parseCategory = def->category;
            categoryInfo[def->category].startIndex = numThings;
            x = margin;
            y = margin;
            maxRowHeight = 0;
        }

        categoryInfo[def->category].count++;

        // Convert underscores.
        for ( char * c = def->name; *c != '\0'; c++ )
        {
            if ( *c == '_' )
                *c = ' ';
        }

        int len = (int)strlen(def->name);
        if ( len > maxLen )
            maxLen = len;

        int lumpIndex = GetIndexOfLumpNamed(editor.iwad, sprite);
        def->patch = LoadPatch(editor.iwad, lumpIndex);

        // Set palette rect

        int scale = categoryInfo[def->category].paletteScale;

        if ( x + def->patch.rect.w * scale > paletteWidth - margin )
        {
            x = margin;
            y += maxRowHeight + margin;
            maxRowHeight = 0;
        }

        if ( def->patch.rect.h * scale > maxRowHeight )
            maxRowHeight = def->patch.rect.h * scale;

        def->patch.rect.x = x;
        def->patch.rect.y = y;

        x += def->patch.rect.w * scale + margin;

        numThings++;
    }

//    for ( int i = 0; i < THING_CATEGORY_COUNT; i++ )
//        printf("category %d start index: %d, count: %d\n",
//               i, categoryInfo[i].startIndex, categoryInfo[i].count);
//    printf("max thing name: %d\n", maxLen);
    fclose(file);

    // Check if there are any duplicates...

#if 0
    printf("Checking for duplicate things...\n");
    bool found = false;
    for ( int i = 0; i < numThings; i++ )
    {
        printf("%s rect: %d, %d, %d, %d\n",
               thingDefs[i].name,
               thingDefs[i].patch.rect.x,
               thingDefs[i].patch.rect.y,
               thingDefs[i].patch.rect.w,
               thingDefs[i].patch.rect.h);
        for ( int j = i + 1; j < numThings; j++ )
            if ( thingDefs[i].doomedType == thingDefs[j].doomedType)
            {
                found = true;
                int type = thingDefs[i].doomedType;
                printf("duplicate thing (%d) in things.dsp:\n", type);
                printf("- index %d: %s\n", i, thingDefs[i].name);
                printf("- index %d: %s\n", j, thingDefs[j].name);
            }
    }
    if ( !found )
        printf("... no duplicates\n");
#endif

    CompareOldThingDefs();

    atexit(FreeThingDefs);
}

ThingDef * GetThingDef(int type)
{
    for ( int i = 0; i < numThings; i++ )
    {
        if ( thingDefs[i].doomedType == type )
            return &thingDefs[i];
    }

    return NULL;
}
