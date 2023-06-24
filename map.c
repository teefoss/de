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

    SDL_Rect bounds = {
        .x = left - BOUNDS_BORDER,
        .y = top - BOUNDS_BORDER,
        .w = right - left + BOUNDS_BORDER * 2,
        .h = bottom - top + BOUNDS_BORDER * 2
    };

    return bounds;
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

    // Load vertices.

    int numVertices = 0;
    mapvertex_t * mapVertices = LoadMapData(wad, lumpLabel, ML_VERTEXES, &numVertices);

    map.vertices = NewArray(numVertices, sizeof(Vertex), 16);
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
//        printf("loaded vertex %3d (%3d, %3d)\n", i, vertex.x, vertex.y);
    }

    Vertex * vertices = map.vertices->data;

    int numLines = 0;
    maplinedef_t * lines = LoadMapData(wad, lumpLabel, ML_LINEDEFS, &numLines);


    mapsidedef_t * sidedefs = LoadMapData(wad, lumpLabel, ML_SIDEDEFS, NULL);

    map.lines = NewArray(numLines, sizeof(Line), 16);
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
        }

        Push(map.lines, &line);
//        printf("loaded line %3d: %3d, %3d\n", i, line.v1, line.v2);
    }

    // After all lines are loaded, check if there are any 'dead' vertices,
    // i.e. those that don't belong to a line.

//    for ( int i = 0; i < map.vertices->count; i++ ) {
//        if ( vertices[i].referenceCount == 0 ) {
//            vertices[i].removed = true;
//        }
//    }

    int numThings = 0;
    mapthing_t * things = LoadMapData(wad, lumpLabel, ML_THINGS, &numThings);

    map.things = NewArray(numThings, sizeof(Thing), 16);
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

    free(mapVertices);
    free(lines);
    free(sidedefs);
    free(things);
}

SDL_Point LineMidpoint(const Line * line)
{
    Vertex * vertices = map.vertices->data;
    SDL_Point p1 = vertices[line->v1].origin;
    SDL_Point p2 = vertices[line->v2].origin;

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;

    return (SDL_Point){ p1.x + dx / 2.0f, p1.y + dy / 2.0f };
}

float LineLength(const Line * line)
{
    Vertex * vertices = map.vertices->data;
    SDL_Point p1 = vertices[line->v1].origin;
    SDL_Point p2 = vertices[line->v2].origin;

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;

    return sqrtf(dx * dx + dy * dy);
}

void GetLinePoints(int index, SDL_Point * p1, SDL_Point * p2)
{
    Line * line = Get(map.lines, index);
    Vertex * vertices = map.vertices->data;

    *p1 = vertices[line->v1].origin;
    *p2 = vertices[line->v2].origin;
}
