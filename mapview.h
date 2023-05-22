//
//  mapview.h
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#ifndef mapview_h
#define mapview_h

#include <SDL2/SDL.h>

extern SDL_Rect visibleRect;

void InitMapView(void);
void UpdateVisibleRectSize(int width, int height);
void DrawMap(void);

#endif /* mapview_h */
