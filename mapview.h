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
extern int gridSize;

void InitMapView(void);
void UpdateVisibleRectSize(int width, int height);
void DrawMap(void);

void ZoomIn(void);
void ZoomOut(void);

#endif /* mapview_h */
