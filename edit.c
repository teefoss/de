//
//  edit.c
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#include "edit.h"

#include "common.h"
#include "mapview.h"
#include <stdbool.h>

//const float scrollSpeed = 256.0f;
//const float scrollAccel = 16.0f;

void EditorLoop(void)
{
    const u8 * keys = SDL_GetKeyboardState(NULL);
    const float dt = 1.0f / GetRefreshRate();

    bool run = true;
    while ( run )
    {
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
