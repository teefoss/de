//
//  panel.h
//  de
//
//  Created by Thomas Foster on 5/27/23.
//

#ifndef panel_h
#define panel_h

#include "common.h"
#include <SDL2/SDL.h>

typedef struct
{
    int x, y;   // Location within Panel (relative)
    int width;  // Total highlight width
    int up;     // Index of item above (-1 if none, 0 if next/prev in items)
    int down;   // Index of item below (-1 if none, 0 if next/prev in items)
    int left;   // Index of item to the left (-1 if none)
    int right;  // Index of item to the right (-1 if none)
} PanelItem;

typedef struct
{
    u8 width;  // Size of panel in text columns
    u8 height; // Size of panel in text rows

    PanelItem * items;
    int numItems;
    int selection;

    SDL_Texture * texture;
    u16 * data;
} Panel;

Panel LoadPanel(const char * path);
void PanelRenderTexture(const Panel * panel, int x, int y);
void SetPanelColor(int index);

#endif /* panel_h */
