//
//  map.c
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "map.h"
#include "wad.h"
#include "doomdata.h"
#include "common.h"

#include <limits.h>

Map map;


#define BOUNDS_BORDER 128

SDL_Rect GetMapBounds(void)
{
    if ( map.boundsDirty )
    {
        int left = INT_MAX;
        int top = INT_MAX;
        int right = INT_MIN;
        int bottom = INT_MIN;

        int max_y = INT_MIN;

        Vertex * v = map.vertices->data;
        for ( int i = 0; i < map.vertices->count; i++, v++ )
        {
            if ( v->origin.y > max_y )
                max_y = v->origin.y;
            if ( v->origin.x < left )
                left = v->origin.x;
            if ( v->origin.y < top )
                top = v->origin.y;
            if ( v->origin.x > right )
                right = v->origin.x;
            if ( v->origin.y > bottom )
                bottom = v->origin.y;
        }

        map.bounds = (SDL_Rect){
            .x = left - BOUNDS_BORDER,
            .y = top - BOUNDS_BORDER,
            .w = right - left + BOUNDS_BORDER * 2,
            .h = bottom - top + BOUNDS_BORDER * 2
        };

        map.boundsDirty = false;
    }

    if ( map.bounds.w < 0 )
    {
        map.bounds.x = 0;
        map.bounds.y = 0;
        map.bounds.w = 0;
        map.bounds.h = 0;
    }

    return map.bounds;
}

/// Don't free the memory returned here, the WAD owns it!
void * GetMapData(const Wad * wad, int labelIndex, int offset, int * count)
{
    Lump * lump = GetLump(wad, labelIndex + offset);

    if ( count )
    {
        size_t size = 0;
        switch ( offset )
        {
            case ML_VERTEXES: size = sizeof(mapvertex_t); break;
            case ML_LINEDEFS: size = sizeof(maplinedef_t); break;
            case ML_SIDEDEFS: size = sizeof(mapsidedef_t); break;
            case ML_SECTORS:  size = sizeof(mapsector_t); break;
            case ML_THINGS:   size = sizeof(mapthing_t); break;
        }

        *count = lump->size / size;
    }

    return lump->data;
}

void CreateMap(const char * label)
{
    strncpy(map.label, label, sizeof(map.label));

    map.vertices = NewArray(0, sizeof(Vertex), 16);
    map.lines = NewArray(0, sizeof(Line), 16);
    map.things = NewArray(0, sizeof(Thing), 16);

    map.boundsDirty = true;
    GetMapBounds();
}

void LoadMap(const Wad * wad, const char * lumpLabel)
{
    strncpy(map.label, lumpLabel, sizeof(map.label));

    int l = GetIndexOfLumpNamed(wad, lumpLabel);
    if ( l == -1 )
    {
        fprintf(stderr, "Bad map label '%s'\n", lumpLabel);
        exit(EXIT_FAILURE);
    }

    int numVertices = 0;
    int numLines = 0;
    int numThings = 0;

    mapvertex_t * vertexData    = GetMapData(wad, l, ML_VERTEXES, &numVertices);
    maplinedef_t * lineData     = GetMapData(wad, l, ML_LINEDEFS, &numLines);
    mapsidedef_t * sidedefData  = GetMapData(wad, l, ML_SIDEDEFS, NULL);
    mapsector_t * sectordefData = GetMapData(wad, l, ML_SECTORS, NULL);
    mapthing_t * thingData      = GetMapData(wad, l, ML_THINGS, &numThings);

    map.vertices = NewArray(numVertices, sizeof(Vertex), 16);
    map.lines = NewArray(numLines, sizeof(Line), 16);
    map.things = NewArray(numThings, sizeof(Thing), 16);


    // Load vertices

    for ( int i = 0; i < numVertices; i++ )
    {
        Vertex vertex =
        {
            .origin = { vertexData[i].x, -vertexData[i].y },
            .referenceCount = 0,
            .removed = true, // This will be updated once a line uses this vert.
        };

        Push(map.vertices, &vertex);
    }


    // Load things

    for ( int i = 0; i < numThings; i++ )
    {
        Thing thing =
        {
            .origin = { thingData[i].x, -thingData[i].y },
            .options = thingData[i].options,
            .angle = thingData[i].angle,
            .type = thingData[i].type
        };

        Push(map.things, &thing);
    }


    // Load Lines

    Vertex * vertices = map.vertices->data;

    for ( int i = 0; i < numLines; i++ )
    {
        Line line = { 0 };

        line.v1 = lineData[i].v1;
        vertices[line.v1].referenceCount++;
        vertices[line.v1].removed = false;

        line.v2 = lineData[i].v2;
        vertices[line.v2].referenceCount++;
        vertices[line.v2].removed = false;

        line.flags = lineData[i].flags;
        line.tag = lineData[i].tag;
        line.special = lineData[i].special;

        // Load side(s)

        for ( int s = 0; s < 2; s++ )
        {
            if ( lineData[i].sidenum[s] == -1 )
                continue;

            Side * side = &line.sides[s];
            memset(side, 0, sizeof(*side));

            mapsidedef_t * mside = &sidedefData[lineData[i].sidenum[s]];
            side->offsetX = mside->textureoffset;
            side->offsetY = mside->rowoffset;
            strncpy(side->bottom, mside->bottomtexture, 8);
            strncpy(side->middle, mside->midtexture, 8);
            strncpy(side->top, mside->toptexture, 8);

            // Load side's sector

            mapsector_t * msec = &sectordefData[mside->sector];
            SectorDef * def = &side->sectorDef;
            memset(def, 0, sizeof(*def));

            def->floorHeight = msec->floorheight;
            def->ceilingHeight = msec->ceilingheight;
            strncpy(def->floorFlat, msec->floorpic, 8);
            strncpy(def->ceilingFlat, msec->ceilingpic, 8);
            def->lightLevel = msec->lightlevel;
            def->special = msec->special;
            def->tag = msec->tag;
        }

        Push(map.lines, &line);
//        printf("loaded line %3d: %3d, %3d\n", i, line.v1, line.v2);
    }

    map.boundsDirty = true;
    GetMapBounds();
}

bool ReadLine(FILE * dwd, SDL_Point * p1, SDL_Point *p2, Line * line)
{
    if ( fscanf(dwd, "(%d,%d) to (%d,%d) : %d : %d : %d\n",
                &p1->x, &p1->y, &p2->x, &p2->y,
                &line->flags, &line->special, &line->tag) != 7 )
    {
        return false;
    }

    int numSides = line->flags & ML_TWOSIDED ? 2 : 1;

    for ( int i = 0; i < numSides; i++ )
    {
        Side * side = &line->sides[i];

        if ( fscanf(dwd, "    %d (%d : %s / %s / %s )\n",
                    &side->offsetY,
                    &side->offsetX,
                    side->top,
                    side->bottom,
                    side->middle) != 5 )
        {
            return false;
        }

        SectorDef * e = &side->sectorDef;

        if ( fscanf(dwd, "    %d : %s %d : %s %d %d %d\n",
                    &e->floorHeight,
                    e->floorFlat,
                    &e->ceilingHeight,
                    e->ceilingFlat,
                    &e->lightLevel,
                    &e->special,
                    &e->tag) != 7 )
        {
            return false;
        }
    }

    return true;
}

void WriteLine(FILE * dwd, Line * line)
{
    Vertex * vertices = map.vertices->data;
    SDL_Point p1 = vertices[line->v1].origin;
    SDL_Point p2 = vertices[line->v2].origin;

    fprintf(dwd, "(%d,%d) to (%d,%d) : %d : %d : %d\n",
            p1.x, p1.y, p2.x, p2.y, line->flags, line->special, line->tag);

    int numSides = line->flags & ML_TWOSIDED ? 2 : 1;

    for ( int i = 0; i < numSides; i++ )
    {
        Side * side = &line->sides[i];

        if ( strlen(side->top) == 0 )
            strcpy(side->top, "-");

        if ( strlen(side->middle) == 0 )
            strcpy(side->middle, "-");

        if ( strlen(side->bottom) == 0 )
            strcpy(side->bottom, "-");

        if ( strlen(side->sectorDef.floorFlat) == 0 )
            strcpy(side->sectorDef.floorFlat, "-");

        if ( strlen(side->sectorDef.ceilingFlat) == 0 )
            strcpy(side->sectorDef.ceilingFlat, "-");

        fprintf(dwd, "    %d (%d : %s / %s / %s )\n",
                side->offsetY,
                side->offsetX,
                side->top,
                side->bottom,
                side->middle);

        SectorDef * e = &side->sectorDef;

        fprintf(dwd, "    %d : %s %d : %s %d %d %d\n",
                e->floorHeight,
                e->floorFlat,
                e->ceilingHeight,
                e->ceilingFlat,
                e->lightLevel,
                e->special,
                e->tag);

    }
}

bool ReadThing(FILE * dwd, Thing * thing)
{
    if ( fscanf(dwd, "(%i,%i, %d) :%d, %d\n",
                &thing->origin.x,
                &thing->origin.y,
                &thing->angle,
                &thing->type,
                &thing->options) != 5 )
    {
        return false;
    }

    thing->origin.x &= -16; // TODO: DoomEd rounds this. Why?
    thing->origin.y &= -16;

    return true;
}

void WriteThing(FILE * dwd, Thing * thing)
{
    fprintf(dwd, "(%d,%d, %d) :%d, %d\n",
            thing->origin.x,
            thing->origin.y,
            thing->angle,
            thing->type,
            thing->options);
}

void LoadDWD(const char * mapName)
{
    char fileName[256] = { 0 };
    strncpy(fileName, mapName, sizeof(fileName) - 1);

    const char * extension = ".dwd";
    strncat(fileName, extension, sizeof(fileName) - strlen(extension) - 1);

    FILE * dwd = fopen(fileName, "r");
    if ( dwd == NULL )
    {
        printf("Error: '%s' not found!\n", fileName);
        return;
    }

    // Check header.

    int version = -1;
    if ( fscanf(dwd, "WorldServer version %d\n", &version) != 1 || version != 4 )
        goto error;

    // Read lines.

    int numLines;
    if ( fscanf(dwd, "\nlines:%d\n", &numLines) != 1 )
        goto error;

    for ( int i = 0; i < numLines; i++ )
    {
        SDL_Point p1, p2;
        Line line = { 0 };

        if ( !ReadLine(dwd, &p1, &p2, &line) )
            goto error;

//        NewLine(&p1, &p2, &line);
    }
    printf("Loaded %d lines.\n", numLines);

    // Read things.

    int numThings;
    if ( fscanf(dwd, "/nthings:%d\n", &numThings) != 1 )
        goto error;

    for ( int i = 0; i < numThings; i++ )
    {
        Thing thing = { 0 };
        if ( !ReadThing(dwd, &thing) )
            goto error;

//        NewThing(&thing);
    }
    printf("Loaded %d things.\n", numThings);

    return;
error:
    printf("Error reading %s!\n", fileName);
}

void SaveDWD(void)
{
    char fileName[80] = { 0 };
    strcpy(fileName, map.label);
    strcat(fileName, ".dwd");

    FILE * dwd = fopen(fileName, "w");
    if ( dwd == NULL )
    {
        printf("Error: failed to create %s!\n", fileName);
        return;
    }

    fprintf(dwd, "WorldServer version 4\n");

    // Lines

    int numLines = 0;
    Line * lines = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++ )
        if ( !lines[i].deleted )
            numLines++;

    fprintf(dwd, "\nlines:%d\n", numLines);

    for ( int i = 0; i < map.lines->count; i++ )
        if ( !lines[i].deleted )
            WriteLine(dwd, &lines[i]);

    // Things

    int numThings = 0;
    Thing * things = map.things->data;
    for ( int i = 0; i < map.things->count; i++ )
        if ( !things[i].deleted )
            numThings++;

    fprintf(dwd, "\nthings:%d\n", numThings);

    for ( int i = 0; i < map.things->count; i++ )
        if ( !things[i].deleted )
            WriteThing(dwd, &things[i]);
}
