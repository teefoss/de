//
//  mapview.h
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#ifndef mapview_h
#define mapview_h

#include <SDL2/SDL.h>

#define VERTEX_DRAW_SIZE 4
#define SELECTION_SIZE 12 // Box width/height around click point
#define THING_DRAW_SIZE 32

extern SDL_FRect visibleRect;
extern int gridSize;
extern float scale;

void InitMapView(void);
void UpdateVisibleRectSize(int width, int height);
void DrawMap(void);
void DrawSelectionBox(const SDL_Rect * box);

SDL_Point WindowToWorld(const SDL_Point * point);
SDL_Point WorldToWindow(const SDL_Point * point);

void ZoomIn(void);
void ZoomOut(void);

#endif /* mapview_h */
