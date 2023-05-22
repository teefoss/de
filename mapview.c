//
//  mapview.c
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#include "mapview.h"
#include "map.h"
#include "window.h"

SDL_Rect visibleRect;

void InitMapView(void)
{
    SDL_Rect bounds = GetMapBounds();

    SDL_GetWindowSize(window, &visibleRect.w, &visibleRect.h);
    visibleRect.x = bounds.x + (bounds.w / 2) - visibleRect.w / 2;
    visibleRect.y = bounds.y + (bounds.h / 2) - visibleRect.h / 2;
}

static SDL_Point WorldToWindow(int x, int y)
{
    SDL_Point translated = {
        .x = x - visibleRect.x,
        .y = y - visibleRect.y
    };

    return translated;
}

static void DrawLine(int x1, int y1, int x2, int y2)
{
    SDL_Point p1 = WorldToWindow(x1, y1);
    SDL_Point p2 = WorldToWindow(x2, y2);

    SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
}

static void FillRect(const SDL_Rect * rect)
{
    SDL_Point translated = WorldToWindow(rect->x, rect->y);

    SDL_Rect translatedRect = {
        .x = translated.x,
        .y = translated.y,
        .w = rect->w,
        .h = rect->h,
    };

    SDL_RenderFillRect(renderer, &translatedRect);
}

static void DrawGrid(void)
{
    int left = visibleRect.x;
    int right = visibleRect.x + visibleRect.w;
    int top = visibleRect.y;
    int bottom = visibleRect.y + visibleRect.h;

    SDL_SetRenderDrawColor(renderer, 192, 192, 255, 255);

    int gridStartX = left;
    while ( gridStartX % 64 != 0 )
        gridStartX++;

    for ( int x = gridStartX; x <= right; x += 64 )
        DrawLine(x, top, x, bottom);

    int gridStartY = top;
    while ( gridStartY % 64 != 0 )
        gridStartY++;

    for ( int y = gridStartY; y <= bottom; y += 64 )
        DrawLine(left, y, right, y);
}

static void DrawPoints(void)
{
    static const int pointSize = 4;

    int left = visibleRect.x - pointSize;
    int right = visibleRect.x + visibleRect.w + pointSize;
    int top = visibleRect.y - pointSize;
    int bottom = visibleRect.y + visibleRect.h + pointSize;

    SDL_Rect pointRect = {
        .w = pointSize,
        .h = pointSize
    };

    SDL_SetRenderDrawColor(renderer, 8, 8, 8, 255);
    Point * p = map.points->data;
    for ( int i = 0; i < map.points->count; i++, p++ )
    {
        if ( p->x < left || p->x > right || p->y < top || p->y > bottom )
            continue;

        pointRect.x = p->x - pointSize / 2;
        pointRect.y = p->y - pointSize / 2;

        FillRect(&pointRect);
    }
}

static void DrawLines(void)
{
    SDL_SetRenderDrawColor(renderer, 8, 8, 8, 255);

    Point * points = map.points->data;
    Line * l = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, l++ ) {
        DrawLine(points[l->v1].x,
                 points[l->v1].y,
                 points[l->v2].x,
                 points[l->v2].y);
    }
}

void DrawMap(void)
{
    DrawGrid();
    DrawLines();
    DrawPoints();
}
