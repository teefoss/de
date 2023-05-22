//
//  window.h
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#ifndef window_h
#define window_h

#include <SDL2/SDL.h>

void InitWindow(void);

extern SDL_Window * window;
extern SDL_Renderer * renderer;

int GetRefreshRate(void);

#endif /* window_h */
