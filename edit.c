//
//  edit.c
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#include "edit.h"

#include "common.h"
#include "mapview.h"
#include "map.h"
#include "geometry.h"

#include <stdbool.h>

#define SHIFT_DOWN (mods & KMOD_SHIFT)

const u8 * keys;
SDL_Keymod mods;
SDL_Point mouse;

// Vertex, Line, Thing, and selection box dragging.
bool draggingObjects;
bool draggingSelectionBox;
bool draggingView;
SDL_Point previousDragPoint;
SDL_Point dragStart;
SDL_Rect selectionBox;

SDL_Point GridPoint(const SDL_Point * worldPoint)
{
    float x = (float)worldPoint->x / (float)gridSize;
    float y = (float)worldPoint->y / (float)gridSize;
    x += worldPoint->x < 0.0f ? -0.5f : 0.5f;
    y += worldPoint->y < 0.0f ? -0.5f : 0.5f;

    SDL_Point gridPoint = { (int)x * gridSize, (int)y * gridSize };

    return gridPoint;
}

void DeselectAllObjects(void)
{
    Vertex * vertices = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++ ) {
        vertices[i].selected = false;
    }

    Line * lines = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++ ) {
        lines[i].selected = false;
    }

    Thing * things = map.things->data;
    for ( int i = 0; i < map.things->count; i++ ) {
        things[i].selected = false;
    }
}

void StartDraggingObjects(void)
{
    draggingObjects = true;
    previousDragPoint = GridPoint(&mouse);
}

void DragSelectedObjects(void)
{
    SDL_Point current = GridPoint(&mouse);

    int dx = current.x - previousDragPoint.x;
    int dy = current.y - previousDragPoint.y;

    Vertex * vertices = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++ )
    {
        if ( vertices[i].removed ) continue;

        if ( vertices[i].selected )
        {
            vertices[i].origin.x += dx;
            vertices[i].origin.y += dy;
        }
    }

    Thing * things = map.things->data;
    for ( int i = 0; i < map.things->count; i++ )
    {
        if ( things[i].selected ) {
            things[i].origin.x += dx;
            things[i].origin.y += dy;
        }
    }

    previousDragPoint = current;
}

static void UpdateSelectionBox(void)
{
    int left    = mouse.x < dragStart.x ? mouse.x : dragStart.x;
    int top     = mouse.y < dragStart.y ? mouse.y : dragStart.y;
    int right   = mouse.x < dragStart.x ? dragStart.x : mouse.x;
    int bottom  = mouse.y < dragStart.y ? dragStart.y : mouse.y;

    selectionBox.x = left;
    selectionBox.y = top;
    selectionBox.w = (right - left) + 1;
    selectionBox.h = (bottom - top) + 1;
    printf("update selbox\n");
}

void SelectObject(void)
{
    SDL_Rect clickRect = MakeCenteredRect(&mouse, SELECTION_SIZE);

    Vertex * vertex = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++, vertex++ )
    {
        if ( vertex->removed ) continue;

        if ( SDL_PointInRect(&vertex->origin, &clickRect) )
        {
            if ( !SHIFT_DOWN && !vertex->selected )
                DeselectAllObjects();

            if ( vertex->selected && SHIFT_DOWN )
            {
                vertex->selected = false;
            }
            else
            {
                vertex->selected = true;
                StartDraggingObjects();
            }

            return;
        }
    }

    Vertex * vertices = map.vertices->data;
    Line * line = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        if ( LineInRect(&vertices[line->v1].origin,
                        &vertices[line->v2].origin,
                        &clickRect) )
        {
            if ( !SHIFT_DOWN && !line->selected )
                DeselectAllObjects();

            if ( SHIFT_DOWN && line->selected )
            {
                line->selected = false;
            }
            else
            {
                line->selected = true;
                vertices[line->v1].selected = true;
                vertices[line->v2]. selected = true;
                StartDraggingObjects();
            }

            return;
        }
    }

    clickRect = MakeCenteredRect(&mouse, THING_DRAW_SIZE);
    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
    {
        if ( SDL_PointInRect(&thing->origin, &clickRect) )
        {
            if ( !SHIFT_DOWN && !thing->selected )
                DeselectAllObjects();

            if ( SHIFT_DOWN && thing->selected )
            {
                thing->selected = false;
            }
            else
            {
                thing->selected = true;
                StartDraggingObjects();
            }

            return;
        }
    }

    if ( !SHIFT_DOWN )
        DeselectAllObjects();

    draggingSelectionBox = true;
    dragStart = mouse;

    // To avoid seeing a frame of the box's previous position.
    UpdateSelectionBox();
}

void SelectObjectsInSelectionBox(void)
{
    SDL_Rect vertexCheckRect = {
        .x = selectionBox.x - VERTEX_DRAW_SIZE / 2,
        .y = selectionBox.y - VERTEX_DRAW_SIZE / 2,
        .w = selectionBox.w + VERTEX_DRAW_SIZE,
        .h = selectionBox.h + VERTEX_DRAW_SIZE
    };

    Vertex * vertex = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++, vertex++ )
    {
        if ( vertex->removed ) continue;
        if ( SDL_PointInRect(&vertex->origin, &vertexCheckRect) )
            vertex->selected = true;
    }

    Vertex * vertices = map.vertices->data;
    Line * line = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        Vertex * v1 = &vertices[line->v1];
        Vertex * v2 = &vertices[line->v2];
        if ( LineInRect(&v1->origin, &v2->origin, &selectionBox) ) {
            v1->selected = true;
            v2->selected = true;
            line->selected = true;
        }
    }

    SDL_Rect thingCheckRect = {
        .x = selectionBox.x - THING_DRAW_SIZE / 2,
        .y = selectionBox.y - THING_DRAW_SIZE / 2,
        .w = selectionBox.w + THING_DRAW_SIZE,
        .h = selectionBox.h + THING_DRAW_SIZE
    };

    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
    {
        if ( SDL_PointInRect(&thing->origin, &thingCheckRect) ) {
            thing->selected = true;
        }
    }
}

void EditorLoop(void)
{
    const u8 * keys = SDL_GetKeyboardState(NULL);

    const float dt = 1.0f / GetRefreshRate();

    int previousMouseX = 0;
    int previousMouseY = 0;

    // TODO: add some test that VSYNC is working, otherwise limit framerate.
    bool run = true;
    while ( run )
    {
        mods = SDL_GetModState();

        int mouseX;
        int mouseY;
        u32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
        mouse = WindowToWorld(&(SDL_Point){ mouseX, mouseY });

        SDL_Event event;
        while ( SDL_PollEvent(&event) )
        {
            switch ( event.type )
            {
                case SDL_QUIT:
                    run = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch ( event.window.event )
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            visibleRect.w = event.window.data1;
                            visibleRect.h = event.window.data2;
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_KEYDOWN:
                    switch ( event.key.keysym.sym )
                    {
                        case SDLK_EQUALS:
                            ZoomIn();
                            break;
                        case SDLK_MINUS:
                            ZoomOut();
                            break;
                        case SDLK_RIGHTBRACKET:
                            if ( gridSize * 2 <= 64 )
                                gridSize *= 2;
                            break;
                        case SDLK_LEFTBRACKET:
                            if ( gridSize / 2 >= 1 )
                                gridSize /= 2;
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    switch ( event.button.button ) {
                        case SDL_BUTTON_LEFT:
                            previousMouseX = mouseX;
                            previousMouseY = mouseY;
                            if ( !keys[SDL_SCANCODE_SPACE] )
                                SelectObject();
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    switch ( event.button.button ) {
                        case SDL_BUTTON_LEFT:
                            UpdateSelectionBox(); // So you don't see the prev.
                            draggingObjects = false;
                            draggingView = false;
                            if ( draggingSelectionBox ) {
                                draggingSelectionBox = false;
                                SelectObjectsInSelectionBox();
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    if ( event.wheel.y < 0 )
                        ZoomIn();
                    else if ( event.wheel.y > 0 )
                        ZoomOut();
                    break;
                default:
                    break;
            }
        }

        if ( (mouseButtons & SDL_BUTTON_LEFT) )
        {
            if ( keys[SDL_SCANCODE_SPACE] )
            {
                if ( !draggingView )
                {
                    draggingView = true;
                    previousMouseX = mouseX;
                    previousMouseY = mouseY;
                }

                int dx = mouseX - previousMouseX;
                int dy = mouseY - previousMouseY;

                visibleRect.x -= dx;
                visibleRect.y -= dy;

                previousMouseX = mouseX;
                previousMouseY = mouseY;
            }
            else if ( draggingObjects )
            {
                DragSelectedObjects();
            }
            else if ( draggingSelectionBox )
            {
                UpdateSelectionBox();
            }
        }

        static float scrollSpeed = 0.0f;

        if (   keys[SDL_SCANCODE_UP]
            || keys[SDL_SCANCODE_DOWN]
            || keys[SDL_SCANCODE_LEFT]
            || keys[SDL_SCANCODE_RIGHT]
            || keys[SDL_SCANCODE_W]
            || keys[SDL_SCANCODE_A]
            || keys[SDL_SCANCODE_S]
            || keys[SDL_SCANCODE_D] )
        {
            scrollSpeed += 60.0f * dt;
            float max = (600.0f / scale) * dt;
            if ( scrollSpeed > max )
                scrollSpeed = max;
        }
        else
        {
            scrollSpeed = 0.0f;
        }

        if ( keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W] )
            visibleRect.y -= scrollSpeed;

        if ( keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S] )
            visibleRect.y += scrollSpeed;

        if ( keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A] )
            visibleRect.x -= scrollSpeed;

        if ( keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D] )
            visibleRect.x += scrollSpeed;

        SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
        SDL_RenderClear(renderer);

        DrawMap();
        if ( draggingSelectionBox )
            DrawSelectionBox(&selectionBox);

        SDL_RenderPresent(renderer);
    }
}
