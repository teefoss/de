//
//  p_stack.c
//  de
//
//  Created by Thomas Foster on 7/25/23.
//

#include "p_stack.h"

static Panel * panelStack[MAX_PANELS];
static int topPanel = -1;
static int mousePanel = -1; // Panel mouse is currently over. TODO: remove?

bool PanelIsUnderneath(const Panel * panel)
{
    return GetPanelStackPosition(panel) < topPanel;
}

void CloseAllPanelsAbove(const Panel * panel)
{
    topPanel = GetPanelStackPosition(panel);
}

int TopPanelStackPosition(void)
{
    return topPanel;
}

void OpenPanel(Panel * panel)
{
    panelStack[++topPanel] = panel;

    // Place it in the middle of the editor window.
    SDL_Rect windowFrame = GetWindowFrame();
    panel->location.x = (windowFrame.w - panel->location.w) / 2;
    panel->location.y = (windowFrame.h - panel->location.h) / 2;
}


void CloseTopPanel(void)
{
    topPanel--;
}


void CloseAllPanels(void)
{
    topPanel = -1;
}


int GetPanelStackPosition(const Panel * panel)
{
    for ( int i = 0; i <= topPanel; i++ )
        if ( panelStack[i] == panel )
            return i;

    return -1;
}


bool PanelStackProcessEvent(const SDL_Event * event)
{
    for ( int i = topPanel; i >= 0; i-- )
    {
        if ( ProcessPanelEvent(panelStack[i], event) )
            return true;
    }

    return false;
}


void RenderPanelStack(void)
{
    for ( int i = 0; i <= topPanel; i++ )
        RenderPanel(panelStack[i]);
}


/// Update the mouse's panel location and which item it's hovering over.
void UpdatePanelMouse(const SDL_Point * windowMouse)
{
    for ( int panelIndex = topPanel; panelIndex >= 0; panelIndex-- )
    {
        Panel * panel = panelStack[panelIndex];
        SDL_Rect rect = PanelRenderLocation(panel);
        panel->selection = -1;
        mousePanel = -1;
        panel->mouseLocation = (SDL_Point){ -1, -1 };

        if ( SDL_PointInRect(windowMouse, &rect) )
        {
            mousePanel = panelIndex;

            panel->mouseLocation.x = windowMouse->x - panel->location.x;
            panel->mouseLocation.y = windowMouse->y - panel->location.y;

            // Mouse text col/row in panel.
            panel->textLocation.x = (windowMouse->x - rect.x) / FONT_WIDTH;
            panel->textLocation.y = (windowMouse->y - rect.y) / FONT_HEIGHT;

            for ( int i = 0; i < panel->numItems; i++ )
            {
                PanelItem * item = &panel->items[i];

                if (   panel->textLocation.y == item->y
                    && panel->textLocation.x >= item->x
                    && panel->textLocation.x < item->x + item->width )
                {
                    panel->selection = i;
                }

                if ( item->hasMouseRect
                    && panel->textLocation.x >= item->mouseX1
                    && panel->textLocation.x <= item->mouseX2
                    && panel->textLocation.y >= item->mouseY1
                    && panel->textLocation.y <= item->mouseY2 )
                {
                    panel->selection = i;
                }
            }
        }
    }

}
