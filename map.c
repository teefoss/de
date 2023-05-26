//
//  map.c
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "map.h"
#include "wad.h"
#include "doomdata.h"

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
        .x = left,
        .y = top,
        .w = right - left,
        .h = bottom - top
    };

    return bounds;
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

    int verticesIndex = labelIndex + ML_VERTEXES;
    mapvertex_t * vertices = GetLumpWithIndex(wad, verticesIndex);
    int numVertices = GetLumpSize(wad, verticesIndex) / sizeof(mapvertex_t);

    map.vertices = NewArray(numVertices, sizeof(Vertex), 16);
    for ( int i = 0; i < numVertices; i++ )
    {
        Vertex vertex = { vertices[i].x, vertices[i].y };
        Push(map.vertices, &vertex);
//        printf("loaded vertex %3d (%3d, %3d)\n", i, vertex.x, vertex.y);
    }

    int linesIndex = labelIndex + ML_LINEDEFS;
    maplinedef_t * lines = GetLumpWithIndex(wad, linesIndex);
    int numLines = GetLumpSize(wad, linesIndex) / sizeof(maplinedef_t);

    map.lines = NewArray(numLines, sizeof(Line), 16);
    for ( int i = 0; i < numLines; i++ )
    {
        Line line = { 0 };
        line.v1 = lines[i].v1;
        line.v2 = lines[i].v2;
        line.flags = lines[i].flags;
        line.tag = lines[i].tag;
        // TODO: sidedefs
        Push(map.lines, &line);
        printf("loaded line %3d: %3d, %3d\n", i, line.v1, line.v2);
    }

    int thingsIndex = labelIndex + ML_THINGS;
    mapthing_t * things = GetLumpWithIndex(wad, thingsIndex);
    int numThings = GetLumpSize(wad, thingsIndex) / sizeof(mapthing_t);

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

    free(vertices);
    free(lines);
    free(things);
}
