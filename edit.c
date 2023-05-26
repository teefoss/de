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

const u8 * keys;
SDL_Keymod mods;
SDL_Point mouse;

SDL_Point GetGridPoint(SDL_Point worldPoint)
{
    float x = (float)worldPoint.x / (float)gridSize;
    float y = (float)worldPoint.y / (float)gridSize;
    x += worldPoint.x < 0.0f ? -0.5f : 0.5f;
    y += worldPoint.y < 0.0f ? -0.5f : 0.5f;

    SDL_Point gridPoint = { (int)x * gridSize, (int)y * gridSize };

    return gridPoint;
}

void DeselectAll(void)
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

void Select(void)
{
//    Box clickBox = MakeCenteredSquare(mouse.x, mouse.y, SELECTION_SIZE);
    SDL_Rect clickRect = MakeCenteredRect(&mouse, SELECTION_SIZE);

    Vertex * vertices = map.vertices->data;
    for ( int i = 0; i < map.vertices->count; i++ )
    {
        Vertex * p = &vertices[i];
        if ( SDL_PointInRect(&p->origin, &clickRect) )
        {
            if ( !(mods & KMOD_SHIFT) )
                DeselectAll();
            
            p->selected = true;
            return;
        }
    }

    Line * lines = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++ )
    {
        Line * l = &lines[i];

        if ( LineInRect(&vertices[l->v1].origin,
                        &vertices[l->v2].origin,
                        &clickRect) )
        {
            if ( !(mods & KMOD_SHIFT) )
                DeselectAll();

            l->selected = true;
            vertices[l->v1].selected = true;
            vertices[l->v2]. selected = true;

            return;
        }
    }

    clickRect = MakeCenteredRect(&mouse, THING_SIZE);

    Thing * thing = map.things->data;
    for ( int i = 0; i < map.things->count; i++, thing++ )
    {
        if ( SDL_PointInRect(&thing->origin, &clickRect) )
        {
            if ( !(mods & KMOD_SHIFT) )
                DeselectAll();

            thing->selected = true;
            return;
        }
    }

    // Nothing was clicked
    DeselectAll();
}

void EditorLoop(void)
{
    const u8 * keys = SDL_GetKeyboardState(NULL);

    const float dt = 1.0f / GetRefreshRate();

    bool run = true;
    while ( run )
    {
        mods = SDL_GetModState();
        SDL_GetMouseState(&mouse.x, &mouse.y);
        mouse = WindowToWorld(&mouse);

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
                            Select();
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }

        static float scrollSpeed = 0.0f;

        if (   keys[SDL_SCANCODE_UP]
            || keys[SDL_SCANCODE_DOWN]
            || keys[SDL_SCANCODE_LEFT]
            || keys[SDL_SCANCODE_RIGHT] )
        {
            scrollSpeed += 64.0f * dt;
            float max = (768.0f / scale) * dt;
            if ( scrollSpeed > max )
            {
                scrollSpeed = max;
            }
        }
        else
        {
            scrollSpeed = 0.0f;
        }

        if ( keys[SDL_SCANCODE_UP] )
            visibleRect.y -= scrollSpeed;

        if ( keys[SDL_SCANCODE_DOWN] )
            visibleRect.y += scrollSpeed;

        if ( keys[SDL_SCANCODE_LEFT] )
            visibleRect.x -= scrollSpeed;

        if ( keys[SDL_SCANCODE_RIGHT] )
            visibleRect.x += scrollSpeed;

        SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
        SDL_RenderClear(renderer);

        DrawMap();

        SDL_RenderPresent(renderer);
    }
}
