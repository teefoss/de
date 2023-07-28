//
//  sector.h
//  de
//
//  Created by Thomas Foster on 6/21/23.
//

#ifndef sector_h
#define sector_h

#include <stdbool.h>
#include <SDL2/SDL.h>

// zoom in/out: o, p
// scroll: arrow keys

//#define DRAW_BLOCK_MAP

#ifdef DRAW_BLOCK_MAP
extern SDL_Window * bmapWindow;
extern SDL_Renderer * bmapRenderer;
extern SDL_Texture * bmapTexture;
extern SDL_Rect bmapLocation;
extern float bmapScale;
#endif

bool SelectSector(const SDL_Point * point);

#endif /* sector_h */
