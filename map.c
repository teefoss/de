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
#include "geometry.h"

#include <limits.h>

Map map;

bool lineCross[9][9];

void InitLineCross(void)
{
    for ( int x1 = 0; x1 < 3; x1++ )
    {
        for ( int y1 = 0; y1 < 3; y1++ )
        {
            for ( int x2 = 0; x2 < 3; x2++ )
            {
                for ( int y2=0 ; y2<3 ; y2++ )
                {
                    if  (   ((x1<=1 && x2>=1) || (x1>=1 && x2<=1))
                         && ((y1<=1 && y2>=1) || (y1>=1 && y2<=1)) )
                    {
                        lineCross[y1 * 3 + x1][y2 * 3 + x2] = true;
                    }
                    else
                    {
                        lineCross[y1 * 3 + x1][y2 * 3 + x2] = false;
                    }
                }
            }
        }
    }
}

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

Side * SelectedSide(Line * line)
{
    if ( line->selected <= 0 )
        return NULL;

    return &line->sides[line->selected - 1];
}

void ClipLine(int lineIndex, SDL_Point * out1, SDL_Point * out2)
{
    SDL_Rect bounds = GetMapBounds();
    SDL_Point p1, p2;
    GetLinePoints(lineIndex, &p1, &p2);

    double x0 = p1.x;
    double y0 = p1.y;
    double x1 = p2.x;
    double y1 = p2.y;
    CohenSutherlandLineClip(&bounds, &x0, &y0, &x1, &y1);

    out1->x = x0;
    out1->y = y0;
    out2->x = x1;
    out2->y = y1;
}

static int PointRegion(const SDL_Point * point)
{
    SDL_Rect bounds = GetMapBounds();
    int x;
    int y;

    if ( point->x < bounds.x )
        x = 0;
    else if ( point->x > bounds.x + bounds.w )
        x = 2;
    else
        x = 1;

    if ( point->y < bounds.y )
        y = 0;
    else if ( point->y > bounds.y + bounds.h )
        y = 2;
    else
        y = 1;

    return y * 3 + x;
}

Visibility LineVisibility(int index)
{
    SDL_Point p1, p2;
    GetLinePoints(index, &p1, &p2);

    int region1 = PointRegion(&p1);
    int region2 = PointRegion(&p2);

    if ( !lineCross[region1][region2] )
        return VISIBILITY_NONE;
    else if ( region1 == 4 && region2 == 4 )
        return VISIBILITY_FULL;
    else
        return VISIBILITY_PARTIAL;

}
