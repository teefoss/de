//
//  mapview.c
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#include "mapview.h"
#include "map.h"
#include "common.h"
#include "geometry.h"
#include "edit.h"

SDL_Rect visibleRect;
float scale = 1.0f;
int gridSize = 8;

static SDL_Point WorldToWindow(const SDL_Point * point)
{
    SDL_Point converted = {
        .x = (point->x - visibleRect.x) * scale,
        .y = (point->y - visibleRect.y) * scale
    };

    return converted;
}

SDL_Point WindowToWorld(const SDL_Point * point)
{
    SDL_Point converted = {
        .x = (point->x / scale) + visibleRect.x,
        .y = (point->y / scale) + visibleRect.y
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

    printf("scale set to %g%%\n", scale * 100.0f);
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

static void DrawLine(const SDL_Point * p1, const SDL_Point * p2)
{
    SDL_Point c1 = WorldToWindow(p1);
    SDL_Point c2 = WorldToWindow(p2);

    SDL_RenderDrawLine(renderer, c1.x, c1.y, c2.x, c2.y);
}

/// Draw a filled rect, accounting for view origin and scale.
static void FillRect(const SDL_Rect * rect)
{
    SDL_Point rectOrigin = { rect->x, rect->y };
    rectOrigin = WorldToWindow(&rectOrigin);

    SDL_Rect translatedRect =
    {
        .x = rectOrigin.x,
        .y = rectOrigin.y,
        .w = rect->w * scale,
        .h = rect->h * scale,
    };

    SDL_RenderFillRect(renderer, &translatedRect);
}

static void SetGridColor(int coordinate)
{
    if ( coordinate % 64 == 0 )
        SDL_SetRenderDrawColor(renderer, 160, 160, 255, 255);
    else
        SDL_SetRenderDrawColor(renderer, 216, 216, 216, 255);
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
        DrawLine(&(SDL_Point){ x, top }, &(SDL_Point){ x, bottom });
    }

    int gridStartY = top;
    while ( gridStartY % gridSize != 0 )
        gridStartY++;

    for ( int y = gridStartY; y <= bottom; y += gridSize )
    {
        SetGridColor(y);
        DrawLine(&(SDL_Point){ left, y }, &(SDL_Point){ right, y });
    }
}

static void DrawPoints(void)
{
    const int pointSize = POINT_SIZE;

    int left = visibleRect.x - pointSize;
    int right = visibleRect.x + visibleRect.w + pointSize;
    int top = visibleRect.y - pointSize;
    int bottom = visibleRect.y + visibleRect.h + pointSize;

    SDL_Rect pointRect = {
        .w = pointSize,
        .h = pointSize
    };

    Vertex * v = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++, v++ )
    {
        if ( v->removed ) continue;
        
        if (   v->origin.x < left
            || v->origin.x > right
            || v->origin.y < top
            || v->origin.y > bottom )
            continue;

        pointRect.x = v->origin.x - POINT_SIZE / 2;
        pointRect.y = v->origin.y - POINT_SIZE / 2;

        if ( v->selected )
            SDL_SetRenderDrawColor(renderer, 248, 64, 64, 255);
        else
            SDL_SetRenderDrawColor(renderer, 8, 8, 8, 255);

        FillRect(&pointRect);
    }
}

// TODO: culling
static void DrawLines(void)
{
    Vertex * vertices = map.vertices->data;
    Line * l = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, l++ ) {
        SDL_Point p1 = vertices[l->v1].origin;
        SDL_Point p2 = vertices[l->v2].origin;

        if ( l->selected ) {
            SDL_SetRenderDrawColor(renderer, 248, 64, 64, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 8, 8, 8, 255);
        }

        DrawLine(&p1, &p2);

        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;

        float length = sqrtf(dx * dx + dy * dy) / 6.0f;
        SDL_Point mid = { p1.x + dx / 2.0f, p1.y + dy / 2.0f };
        SDL_Point normal = { mid.x - dy / length , mid.y + dx / length };

        DrawLine(&mid, &normal);
    }
}

static void DrawThings(void)
{
    const int thingSize = THING_SIZE;

    int left = visibleRect.x - thingSize;
    int right = visibleRect.x + visibleRect.w + thingSize;
    int top = visibleRect.y - thingSize;
    int bottom = visibleRect.y + visibleRect.h + thingSize;

    SDL_Rect thingRect = {
        .w = thingSize,
        .h = thingSize
    };

    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ ) {
        if (   thing->origin.x < left
            || thing->origin.x > right
            || thing->origin.y < top
            || thing->origin.y > bottom )
            continue;

        thingRect.x = thing->origin.x - THING_SIZE / 2;
        thingRect.y = thing->origin.y - THING_SIZE / 2;

        if ( thing->selected )
            SDL_SetRenderDrawColor(renderer, 248, 64, 64, 255);
        else
            SDL_SetRenderDrawColor(renderer, 8, 8, 8, 255);

        FillRect(&thingRect);
    }
}

void DrawMap(void)
{
    DrawGrid();
    DrawLines();
    DrawPoints();
    DrawThings();

    // Debug, show click rect.
#if 0
    Box debug_mouse = MakeCenteredSquare(mouse.x, mouse.y, SELECTION_SIZE);
    SDL_Rect r = {
        debug_mouse.left,
        debug_mouse.top,
        (debug_mouse.right - debug_mouse.left) + 1,
        (debug_mouse.bottom - debug_mouse.top) + 1,
    };

    SDL_Point convert = WorldToWindow(r.x, r.y);
    r.x = convert.x;
    r.y = convert.y;
    SDL_RenderDrawRect(renderer, &r);
#endif
}
