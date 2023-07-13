//
//  mapview.h
//  de
//
//  Created by Thomas Foster on 5/21/23.
//

#ifndef mapview_h
#define mapview_h

#include <SDL2/SDL.h>

#define VERTEX_DRAW_SIZE 4.0f
#define SELECTION_SIZE 12 // Box width/height around click point
#define THING_DRAW_SIZE 32.0f

extern SDL_FRect visibleRect;
extern int gridSize;
extern float scale;

void InitMapView(void);
void UpdateVisibleRectSize(int width, int height);
void DrawMap(void);
void DrawSelectionBox(const SDL_Rect * box);
void DrawVertex(const SDL_Point * origin);

void WorldDrawLine(const SDL_FPoint * p1, const SDL_FPoint * p2);

SDL_Point WindowToWorld(const SDL_Point * point);
SDL_FPoint WorldToWindow(const SDL_FPoint * point);

void ZoomIn(void);
void ZoomOut(void);

#endif /* mapview_h */
