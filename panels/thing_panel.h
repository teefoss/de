//
//  thing_panel.h
//  de
//
//  Created by Thomas Foster on 6/19/23.
//

#ifndef thing_panel_h
#define thing_panel_h

#include "panel.h"
#include "thing.h"

#define THING_PALETTE_SCALE 2.0f

extern Panel thingPanel;
extern Panel thingPalette;
extern SDL_Rect thingPaletteRect;

void LoadThingPanel(void);

#endif /* thing_panel_h */
