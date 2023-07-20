//
//  line.c
//  de
//
//  Created by Thomas Foster on 6/27/23.
//

#include "m_line.h"
#include "m_map.h"
#include "e_geometry.h"
#include "e_map_view.h"

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

SDL_FPoint LineMidpoint(const Line * line)
{
    Vertex * vertices = map.vertices->data;
    SDL_FPoint p1 = { vertices[line->v1].origin.x, vertices[line->v1].origin.y };
    SDL_FPoint p2 = { vertices[line->v2].origin.x, vertices[line->v2].origin.y };

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;

    return (SDL_FPoint){ p1.x + dx / 2.0f, p1.y + dy / 2.0f };
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

/// Do not pass a line of zero length!
SDL_FPoint LineNormal(const Line * line, float length)
{
    Vertex * vertices = map.vertices->data;
    SDL_FPoint p1 = { vertices[line->v1].origin.x, vertices[line->v1].origin.y };
    SDL_FPoint p2 = { vertices[line->v2].origin.x, vertices[line->v2].origin.y };

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;

    float length1 = LineLength(line) / length;
    SDL_FPoint mid = LineMidpoint(line);
    SDL_FPoint normal = {
        mid.x - dy / length1,
        mid.y + dx / length1
    };

    return normal;
}

void GetLinePoints(int index, SDL_Point * p1, SDL_Point * p2)
{
    Line * line = Get(map.lines, index);
    Vertex * vertices = map.vertices->data;

    *p1 = vertices[line->v1].origin;
    *p2 = vertices[line->v2].origin;
}

Sidedef * SelectedSide(Line * line)
{
    if ( line->selected == DESELECTED )
        return NULL;

    if ( line->deleted )
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
    int x;
    int y;

    if ( point->x < visibleRect.x )
        x = 0;
    else if ( point->x > visibleRect.x + visibleRect.w )
        x = 2;
    else
        x = 1;

    if ( point->y < visibleRect.y )
        y = 0;
    else if ( point->y > visibleRect.y + visibleRect.h )
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
