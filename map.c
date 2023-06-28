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


/// Translate all vertices from NeXTSTEP to SDL coordinate system or vice versa.
static void TranslateAllPoints(void)
{
    SDL_Rect bounds = GetMapBounds();
    int maxY = bounds.h - bounds.y;

    Vertex * vertex = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++, vertex++ )
        vertex->origin.y = maxY - vertex->origin.y;

    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
        thing->origin.y = maxY - thing->origin.y;
}

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

    return map.bounds;
}

void * LoadMapData(const Wad * wad, const char * lumpName, int dataType, int * count)
{
    int labelIndex = GetLumpIndex(wad, lumpName);
    int dataIndex = labelIndex + dataType;

    size_t dataSize = 0;
    switch ( dataType )
    {
        case ML_THINGS:   dataSize = sizeof(mapthing_t); break;
        case ML_LINEDEFS: dataSize = sizeof(maplinedef_t); break;
        case ML_SIDEDEFS: dataSize = sizeof(mapsidedef_t); break;
        case ML_VERTEXES: dataSize = sizeof(mapvertex_t); break;
        case ML_SECTORS:  dataSize = sizeof(mapsector_t); break;
        default:
//            ASSERT("Bad map data type!");
            if ( count ) *count = 0;
            return NULL;
    }

    if ( count )
        *count = GetLumpSize(wad, dataIndex) / dataSize;

    return GetLumpWithIndex(wad, dataIndex);
}

void LoadMap(const Wad * wad, const char * lumpLabel)
{
    strncpy(map.label, lumpLabel, sizeof(map.label));

    int labelIndex = GetLumpIndex(wad, lumpLabel);
    if ( labelIndex == -1 )
    {
        fprintf(stderr, "Bad map label '%s'\n", lumpLabel);
        exit(EXIT_FAILURE);
    }

    int numVertices = 0;
    int numLines = 0;
    int numThings = 0;

    mapvertex_t * mapVertices = LoadMapData(wad, lumpLabel, ML_VERTEXES, &numVertices);
    maplinedef_t * lines = LoadMapData(wad, lumpLabel, ML_LINEDEFS, &numLines);
    mapsidedef_t * sidedefs = LoadMapData(wad, lumpLabel, ML_SIDEDEFS, NULL);
    mapsector_t * sectordefs = LoadMapData(wad, lumpLabel, ML_SECTORS, NULL);
    mapthing_t * things = LoadMapData(wad, lumpLabel, ML_THINGS, &numThings);

    map.vertices = NewArray(numVertices, sizeof(Vertex), 16);
    map.lines = NewArray(numLines, sizeof(Line), 16);
    map.things = NewArray(numThings, sizeof(Thing), 16);


    // Load vertices

    for ( int i = 0; i < numVertices; i++ )
    {
        Vertex vertex =
        {
            .origin.x = mapVertices[i].x,
            .origin.y = mapVertices[i].y,
            .referenceCount = 0,
            .removed = true, // This will be updated once a line uses this vert.
        };

        Push(map.vertices, &vertex);
    }

    // Load Lines

    Vertex * vertices = map.vertices->data;

    for ( int i = 0; i < numLines; i++ )
    {
        Line line = { 0 };

        line.v1 = lines[i].v1;
        vertices[line.v1].referenceCount++;
        vertices[line.v1].removed = false;

        line.v2 = lines[i].v2;
        vertices[line.v2].referenceCount++;
        vertices[line.v2].removed = false;

        line.flags = lines[i].flags;
        line.tag = lines[i].tag;
        line.special = lines[i].special;

        for ( int s = 0; s < 2; s++ )
        {
            // Load side

            Side * side = &line.sides[s];
            mapsidedef_t * mside = &sidedefs[lines[i].sidenum[s]];

            side->offsetX = mside->textureoffset;
            side->offsetY = mside->rowoffset;
            strncpy(side->bottom, mside->bottomtexture, 8);
            side->bottom[8] = '\0';
            strncpy(side->middle, mside->midtexture, 8);
            side->middle[8] = '\0';
            strncpy(side->top, mside->toptexture, 8);
            side->top[8] = '\0';

            // Load side's sector

            mapsector_t * msec = &sectordefs[mside->sector];
            side->sector.floorHeight = msec->floorheight;
            side->sector.ceilingHeight = msec->ceilingheight;

            strncpy(side->sector.floorFlat, msec->floorpic, 8);
            side->sector.floorFlat[8] = '\0';

            strncpy(side->sector.ceilingFlat, msec->ceilingpic, 8);
            side->sector.ceilingFlat[8] = '\0';

            side->sector.lightLevel = msec->lightlevel;
            side->sector.special = msec->special;
            side->sector.tag = msec->tag;
        }

        Push(map.lines, &line);
//        printf("loaded line %3d: %3d, %3d\n", i, line.v1, line.v2);
    }

    // Load things

    for ( int i = 0; i < numThings; i++ ) {
        Thing thing = { 0 };
        thing.origin.x = things[i].x;
        thing.origin.y = things[i].y;
        thing.options = things[i].options;
        thing.angle = things[i].angle;
        thing.type = things[i].type;
        
        Push(map.things, &thing);
    }

    TranslateAllPoints();

    map.boundsDirty = true;
    GetMapBounds();

    free(sectordefs);
    free(mapVertices);
    free(lines);
    free(sidedefs);
    free(things);
}
