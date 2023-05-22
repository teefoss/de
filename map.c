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

/// Translate all points from NeXTSTEP to SDL coordinate system or vice versa.
static void TranslateAllPoints(void)
{
    SDL_Rect bounds;
    int maxY = bounds.h - bounds.y;

    Point * point = map.points->data;
    for ( int i = 0; i < map.points->count; i++, point++ )
        point->y = maxY - point->y;

    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
        thing->y = maxY - thing->y;
}

SDL_Rect GetMapBounds(void)
{

    int left = INT_MAX;
    int top = INT_MAX;
    int right = INT_MIN;
    int bottom = INT_MIN;

    int max_y = INT_MIN;

    Point * p = map.points->data;
    for ( int i = 0; i < map.points->count; i++, p++ )
    {
        if ( p->y > max_y )
            max_y = p->y;
        if ( p->x < left )
            left = p->x;
        if ( p->y < top )
            top = p->y;
        if ( p->x > right )
            right = p->x;
        if ( p->y > bottom )
            bottom = p->y;
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

    map.points = NewArray(numVertices, sizeof(Point), 16);
    for ( int i = 0; i < numVertices; i++ )
    {
        Point point = { vertices[i].x, vertices[i].y };
        Push(map.points, &point);
        printf("loaded vertex %3d (%3d, %3d)\n", i, point.x, point.y);
    }

    int linesIndex = labelIndex + ML_LINEDEFS;
    maplinedef_t * lines = GetLumpWithIndex(wad, linesIndex);
    int numLines = GetLumpSize(wad, linesIndex) / sizeof(maplinedef_t);

    map.lines = NewArray(numLines, sizeof(Line), 16);
    for ( int i = 0; i < numLines; i++ )
    {
        Line line;
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
        Thing thing;
        thing.x = things[i].x;
        thing.y = things[i].y;
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
