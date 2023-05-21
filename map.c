//
//  map.c
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "map.h"
#include "wad.h"
#include "doomdata.h"

Map * LoadMap(const Wad * wad, const char * lumpLabel)
{
    Map * map = malloc(sizeof(*map));

    if ( map == NULL )
        return NULL;

    strncpy(map->label, lumpLabel, sizeof(map->label));

    int labelIndex = GetLumpIndex(wad, lumpLabel);
    if ( labelIndex == -1 )
    {
        fprintf(stderr, "Bad map label '%s'\n", lumpLabel);
        return NULL;
    }

    int verticesIndex = labelIndex + ML_VERTEXES;
    mapvertex_t * vertices = GetLumpWithIndex(wad, verticesIndex);
    int numVertices = GetLumpSize(wad, verticesIndex) / sizeof(mapvertex_t);

    map->points = NewArray(numVertices, sizeof(Point), 16);
    for ( int i = 0; i < numVertices; i++ ) {
        Point point = { vertices[i].x, vertices[i].y };
        Push(map->points, &point);
        printf("loaded vertex %3d (%3d, %3d)\n", i, point.x, point.y);
    }

    free(vertices);

    int linesIndex = labelIndex + ML_LINEDEFS;
    maplinedef_t * lines = GetLumpWithIndex(wad, linesIndex);
    int numLines = GetLumpSize(wad, linesIndex) / sizeof(maplinedef_t);

    map->lines = NewArray(numLines, sizeof(Line), 16);
    for ( int i = 0; i < numLines; i++ ) {
        Line line;
        line.v1 = lines[i].v1;
        line.v2 = lines[i].v2;
        line.flags = lines[i].flags;
        line.tag = lines[i].tag;
        // TODO: sidedefs
        printf("loaded line %3d: %3d, %3d\n", i, line.v1, line.v2);
    }

    return map;
}
