//
//  p_stack.h
//  de
//
//  Created by Thomas Foster on 7/25/23.
//

#ifndef p_stack_h
#define p_stack_h

#include "p_panel.h"
#include <stdbool.h>
#include <SDL2/SDL.h>

void OpenPanel(Panel * panel);
void CloseTopPanel(void);
void CloseAllPanels(void);
void CloseAllPanelsAbove(const Panel * panel);

bool PanelIsUnderneath(const Panel * panel);
int  TopPanelStackPosition(void);
int  GetPanelStackPosition(const Panel * panel);

void RenderPanelStack(void);
bool PanelStackProcessEvent(const SDL_Event * event);

void UpdatePanelMouse(const SDL_Point * windowMouse);

#endif /* p_stack_h */
