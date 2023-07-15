//
//  mapview.c
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#include "e_map_view.h"
#include "m_map.h"
#include "common.h"
#include "e_geometry.h"
#include "e_editor.h"
#include "doomdata.h"
#include "e_defaults.h"
#include "text.h"
#include "e_sector.h"

#define FILLED (-1)

SDL_FRect visibleRect;
float scale = 1.0f;
int gridSize = 8;

SDL_FPoint WorldToWindow(const SDL_FPoint * point)
{
    SDL_FPoint converted = {
        .x = (point->x - (float)visibleRect.x) * scale,
        .y = (point->y - (float)visibleRect.y) * scale
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

    SDL_FRect oldVisibleRect = visibleRect;

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

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    visibleRect.w = w;
    visibleRect.h = h;
    visibleRect.x = bounds.x + (bounds.w / 2) - visibleRect.w / 2;
    visibleRect.y = bounds.y + (bounds.h / 2) - visibleRect.h / 2;
}

void WorldDrawLine(const SDL_FPoint * p1, const SDL_FPoint * p2)
{
    SDL_FPoint c1 = WorldToWindow(p1);
    SDL_FPoint c2 = WorldToWindow(p2);

//    FosterDrawLineAA(c1.x, c1.y, c2.x, c2.y);
//    draw_line_antialias(c1.x, c1.y, c2.x, c2.y);
    SDL_RenderDrawLine(renderer, c1.x, c1.y, c2.x, c2.y);
//    GuptaSprollDrawLine(c1.x, c1.y, c2.x, c2.y);
//    WuDrawLine(c1.x, c1.y, c2.x, c2.y);
}

/// Draw a filled rect, accounting for view origin and scale.
static void WorldDrawRect(const SDL_FRect * rect, int thinkness)
{
    SDL_FPoint rectOrigin = { rect->x, rect->y };
    rectOrigin = WorldToWindow(&rectOrigin);

    SDL_FRect translatedRect =
    {
        .x = rectOrigin.x,
        .y = rectOrigin.y,
        .w = rect->w * scale,
        .h = rect->h * scale,
    };

    if ( thinkness == FILLED ) {
        SDL_RenderFillRectF(renderer, &translatedRect);
    } else {
        DrawRect(&translatedRect, thinkness);
    }
}

static void DrawGrid(void)
{
    float left = visibleRect.x;
    float right = visibleRect.x + visibleRect.w;
    float top = visibleRect.y;
    float bottom = visibleRect.y + visibleRect.h;

    int gridStartX;
    int gridStartY;

    // Don't draw grid lines if too dense.
    if ( gridSize * scale >= 4 )
    {
        gridStartX = left;
        while ( gridStartX % gridSize != 0 )
            gridStartX++;

        SDL_Color gridColor = DefaultColor(GRID_LINES);
        SetRenderDrawColor(&gridColor);

        for ( int x = gridStartX; x <= right; x += gridSize )
        {
            if ( x % 64 == 0 )
                continue;
            WorldDrawLine(&(SDL_FPoint){ x, top },
                          &(SDL_FPoint){ x, bottom });
        }

        gridStartY = top;
        while ( gridStartY % gridSize != 0 )
            gridStartY++;

        for ( int y = gridStartY; y <= bottom; y += gridSize )
        {
            if ( y % 64 == 0 )
                continue;
            WorldDrawLine(&(SDL_FPoint){ left, y },
                          &(SDL_FPoint){ right, y });

        }
    }

    // Tile Grid

    if ( scale > 4.0f / 64 )
    {
        gridStartX = left;
        while ( gridStartX % 64 != 0 )
            gridStartX++;

        SDL_Color tileColor = DefaultColor(GRID_TILES);
        SetRenderDrawColor(&tileColor);

        for ( int x = gridStartX; x <= right; x += 64 )
        {
            WorldDrawLine(&(SDL_FPoint){ x, top },
                          &(SDL_FPoint){ x, bottom });
        }

        gridStartY = top;
        while ( gridStartY % 64 != 0 )
            gridStartY++;

        for ( int y = gridStartY; y <= bottom; y += 64 )
        {
            WorldDrawLine(&(SDL_FPoint){ left, y },
                          &(SDL_FPoint){ right, y });
        }
    }
}

void DrawVertex(const SDL_Point * origin)
{
    const float size = VERTEX_DRAW_SIZE / scale;

    SDL_FRect rect = {
        (float)origin->x - size / 2.0f,
        (float)origin->y - size / 2.0f,
        size,
        size
    };

    WorldDrawRect(&rect, FILLED);
}

static void DrawVertices(void)
{
    const float pointSize = VERTEX_DRAW_SIZE / scale;

    float left = visibleRect.x - pointSize;
    float right = visibleRect.x + visibleRect.w + pointSize;
    float top = visibleRect.y - pointSize;
    float bottom = visibleRect.y + visibleRect.h + pointSize;

    Vertex * v;
    FOR_EACH(v, map.vertices)
    {
        if ( v->removed ) continue;
        
        if (   v->origin.x < left
            || v->origin.x > right
            || v->origin.y < top
            || v->origin.y > bottom )
            continue;

        SDL_Color color;

        if ( v->selected )
            color = DefaultColor(SELECTION);
        else
            color = DefaultColor(VERTEX);

        SetRenderDrawColor(&color);

        DrawVertex(&v->origin);
    }
}

static void DrawLines(void)
{
    Vertex * vertices = map.vertices->data;
    Line * l = map.lines->data;

    for ( int i = 0; i < map.lines->count; i++, l++ )
    {
        if ( l->deleted )
            continue;

        SDL_Point p1 = vertices[l->v1].origin;
        SDL_Point p2 = vertices[l->v2].origin;

        SDL_FPoint clipped1 = { p1.x, p1.y };
        SDL_FPoint clipped2 = { p2.x, p2.y };

        Visibility visibility = LineVisibility(i);

        if ( visibility == VISIBILITY_NONE )
            continue;

        SDL_Color color;

        if ( l->selected )
            color = DefaultColor(SELECTION);
        else if ( l->special > 0 )
            color = DefaultColor(DEF_COLOR_LINE_TWO_SIDED);
        else if ( l->flags & ML_TWOSIDED )
            color = DefaultColor(DEF_COLOR_LINE_TWO_SIDED);
        else
            color = DefaultColor(LINE_ONE_SIDED);

        SetRenderDrawColor(&color);
        WorldDrawLine(&(SDL_FPoint){ clipped1.x, clipped1.y },
                      &(SDL_FPoint){ clipped2.x, clipped2.y });

        SDL_FPoint mid = LineMidpoint(l);
        SDL_FPoint normal = LineNormal(l, 6.0f);
        WorldDrawLine(&mid, &normal);

        // DEBUG: draw an 'F' or 'B' to show which side is selected.
#ifdef DRAW_BLOCK_MAP
        if ( l->selected > 0 ) {
            SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
            SDL_FPoint p = WorldToWindow(&normal);
            RenderChar(p.x - 4, p.y - 8, l->selected == FRONT_SELECTED ? 'F' : 'B' );
        }
#endif
    }
}

static void
DrawThings(void) // TODO: sort selected and draw on top
{
    const int thingSize = THING_DRAW_SIZE;

    int left = visibleRect.x - thingSize;
    int right = visibleRect.x + visibleRect.w + thingSize;
    int top = visibleRect.y - thingSize;
    int bottom = visibleRect.y + visibleRect.h + thingSize;

    SDL_FRect thingRect = {
        .w = thingSize,
        .h = thingSize
    };

    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
    {
        if ( thing->deleted )
            continue;
        
        if (   thing->origin.x < left
            || thing->origin.x > right
            || thing->origin.y < top
            || thing->origin.y > bottom )
            continue;

        thingRect.x = (float)thing->origin.x - THING_DRAW_SIZE / 2.0f;
        thingRect.y = (float)thing->origin.y - THING_DRAW_SIZE / 2.0f;

        ThingDef * def = GetThingDef(thing->type);
        SDL_Color color;
        if ( thing->selected )
            color = DefaultColor(SELECTION);
        else
            color = DefaultColor(def->category + THING_PLAYER);

        SetRenderDrawColor(&color);
        WorldDrawRect(&thingRect, FILLED);
    }
}

void DrawMap(void)
{
    DrawGrid();
    DrawLines();
    DrawVertices();
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

void DrawSelectionBox(const SDL_Rect * box)
{
    if ( box->w <= 0 || box->h <= 0 )
        return;

    SDL_Color color = DefaultColor(SELECTION_BOX);
    color.a = 128;
    SetRenderDrawColor(&color);
    WorldDrawRect(&(SDL_FRect){ box->x, box->y, box->w, box->h }, 4);
}
