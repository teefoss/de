//
//  mapview.h
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#ifndef mapview_h
#define mapview_h

#include <SDL2/SDL.h>

#define POINT_SIZE 4
#define SELECTION_SIZE 7
#define THING_SIZE 32

extern SDL_Rect visibleRect;
extern int gridSize;
extern float scale;

void InitMapView(void);
void UpdateVisibleRectSize(int width, int height);
void DrawMap(void);

SDL_Point WindowToWorld(const SDL_Point * point);

void ZoomIn(void);
void ZoomOut(void);

#endif /* mapview_h */
