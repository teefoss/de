//
//  panels.h
//  de
//
//  Created by Thomas Foster on 5/28/23.
//
//  Header for all x_panel.c

#ifndef panels_h
#define panels_h

#include "map.h"
#include "panel.h"
#include <stdbool.h>

#define PANEL_DIRECTORY "panels/"

typedef bool (* PanelEventHandler)(const SDL_Event *, void *);

extern Panel line_panel;

void LoadLinePanel(void);
bool HandleLinePanelEvent(const SDL_Event * event, void * data);
void RenderLinePanel(const Line * line);

#endif /* panels_h */
