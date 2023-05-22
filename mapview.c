//
//  mapview.c
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#include "mapview.h"
#include "map.h"
#include "common.h"

#define POINT_SIZE 4
#define THING_SIZE 32

SDL_Rect visibleRect;
float scale = 1.0f;
int gridSize = 8;

static SDL_Point WorldToWindow(int x, int y)
{
    SDL_Point converted = {
        .x = (x - visibleRect.x) * scale,
        .y = (y - visibleRect.y) * scale
    };

    return converted;
}

static void PerformZoom(float factor)
{
    float oldScale = scale;

    scale *= factor;

    float min = 0.0625f;
    float max = 4.0f;

    if (scale < min) {
        scale = min;
    } else if (scale > max) {
        scale = max;
    }

    // Adjust the visible rect, keeping the center focus point the same.

    SDL_Rect oldVisibleRect = visibleRect;

    visibleRect.w /= (scale / oldScale);
    visibleRect.h /= (scale / oldScale);

    visibleRect.x += (oldVisibleRect.w - visibleRect.w) / 2;
    visibleRect.y += (oldVisibleRect.h - visibleRect.h) / 2;
}

void ZoomIn(void)
{
    PerformZoom(2.0f);
}

void ZoomOut(void)
{
    PerformZoom(0.5f);
}

void InitMapView(void)
{
    SDL_Rect bounds = GetMapBounds();

    SDL_GetWindowSize(window, &visibleRect.w, &visibleRect.h);
    visibleRect.x = bounds.x + (bounds.w / 2) - visibleRect.w / 2;
    visibleRect.y = bounds.y + (bounds.h / 2) - visibleRect.h / 2;
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

static void SetGridColor(int coordinate)
{
    if ( coordinate % 64 == 0 ) {
        SDL_SetRenderDrawColor(renderer, 160, 160, 255, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 216, 216, 216, 255);
    }
}

static void DrawGrid(void)
{
    int left = visibleRect.x;
    int right = visibleRect.x + visibleRect.w;
    int top = visibleRect.y;
    int bottom = visibleRect.y + visibleRect.h;

    SDL_SetRenderDrawColor(renderer, 192, 192, 255, 255);

    int gridStartX = left;
    while ( gridStartX % gridSize != 0 )
        gridStartX++;

    for ( int x = gridStartX; x <= right; x += gridSize )
    {
        SetGridColor(x);
        DrawLine(x, top, x, bottom);
    }

    int gridStartY = top;
    while ( gridStartY % gridSize != 0 )
        gridStartY++;

    for ( int y = gridStartY; y <= bottom; y += gridSize )
    {
        SetGridColor(y);
        DrawLine(left, y, right, y);
    }

#if 0
    float left = visibleRect.x - 1;
    float right = visibleRect.x + visibleRect.w + 2;
    float top = visibleRect.y - 1;
    float bottom = visibleRect.y + visibleRect.h + 2;

    SDL_SetRenderDrawColor(renderer, 216, 216, 216, 255);

    if ( gridSize * scale >= 4 )
    {
        int xStart  = floorf(left / gridSize);
        int xEnd    = floorf(right / gridSize);
        int yStart  = floorf(top / gridSize);
        int yEnd    = floorf(bottom / gridSize);

        xStart  *= gridSize;
        xEnd    *= gridSize;
        yStart  *= gridSize;
        yEnd    *= gridSize;

        if ( yStart < top )
            yStart += gridSize;
        if ( yEnd >= bottom )
            yEnd -= gridSize;
        if ( xStart < left )
            xStart += gridSize;
        if ( xEnd >= right )
            xEnd -= gridSize;

        for ( int y = yStart; y <= yEnd; y += gridSize )
            if ( y & 63 ) // `y % 64 != 0` but handles negative values.
                DrawLine(left, y, right, y);

        for ( int x = xStart; x <= xEnd; x += gridSize )
            if ( x & 63 )
                DrawLine(x, top, x, bottom);
    }

    SDL_SetRenderDrawColor(renderer, 160, 160, 255, 255);

    if (scale > 4.0 / 64)
    {
        int yStart  = floor(top/64);
        int yEnd    = floor(bottom/64);
        int xStart  = floor(left/64);
        int xEnd    = floor(right/64);

        yStart *= 64;
        yEnd   *= 64;
        xStart *= 64;
        xEnd   *= 64;

        if ( yStart < top )
            yStart += 64;
        if ( xStart < left )
            xStart += 64;
        if (xEnd >= right)
            xEnd -= 64;
        if (yEnd >= bottom)
            yEnd -= 64;

        for ( int y = yStart; y <= yEnd ; y += 64 )
            DrawLine(left, y, right, y);

        for ( int x = xStart; x <= xEnd ; x += 64 )
            DrawLine(x, top, x, bottom);
    }
#endif
}

static void DrawPoints(void)
{
    const int pointSize = (float)POINT_SIZE * scale;

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

        pointRect.x = p->x - POINT_SIZE / 2;
        pointRect.y = p->y - POINT_SIZE / 2;

        FillRect(&pointRect);
    }
}

// TODO: culling
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

static void DrawThings(void)
{
    const int thingSize = (float)THING_SIZE * scale;

    int left = visibleRect.x - thingSize;
    int right = visibleRect.x + visibleRect.w + thingSize;
    int top = visibleRect.y - thingSize;
    int bottom = visibleRect.y + visibleRect.h + thingSize;

    SDL_SetRenderDrawColor(renderer, 8, 8, 8, 255);

    SDL_Rect thingRect = {
        .w = thingSize,
        .h = thingSize
    };


    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ ) {
        if (   thing->x < left
            || thing->x > right
            || thing->y < top
            || thing->y > bottom )
            continue;

        thingRect.x = thing->x - THING_SIZE / 2;
        thingRect.y = thing->y - THING_SIZE / 2;

        FillRect(&thingRect);
    }
}

void DrawMap(void)
{
    DrawGrid();
    DrawLines();
    DrawPoints();
    DrawThings();
}
