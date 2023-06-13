//
//  panel.h
//  de
//
//  Created by Thomas Foster on 5/27/23.
//
//  Generic panel structure and functions.

#ifndef panel_h
#define panel_h

#include "common.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <stdbool.h>

#define PANEL_DIRECTORY "panels/"
#define MAX_PANELS 10
#define MAX_TEXT 256

#define PANEL_PRINT(x, y, ...) \
    PrintString((x) * FONT_WIDTH, (y) * FONT_HEIGHT, __VA_ARGS__)

// TODO: type? i.e. radio, toggle, input
typedef struct
{
    int x, y;   // Location within Panel (relative)
    int width;  // Total highlight width
    int up;     // Index of item above (-1 if none, 0 if next/prev in items)
    int down;   // Index of item below (-1 if none, 0 if next/prev in items)
    int left;   // Index of item to the left (-1 if none)
    int right;  // Index of item to the right (-1 if none)
} PanelItem;

typedef struct panel
{
    u8 width;  // Size of panel in text columns
    u8 height; // Size of panel in text rows

    SDL_Rect location; // In window space, or relative to parent if non-NULL.
    struct panel * parent;

    PanelItem * items;
    int numItems;
    int selection;
    int mouseHover; // Index of item the mouse is over.

    SDL_Texture * texture;
    u16 * consoleData;

    void * data;

    char text[MAX_TEXT];
    int * value;
    int cursor;
    bool isTextEditing;
    int textItem; // The item where text is current being editing.

    bool (* processEvent)(const SDL_Event *);
    void (* render)(void);
} Panel;

extern Panel * openPanels[MAX_PANELS];
extern int topPanel;

Panel LoadPanel(const char * path);
bool ProcessPanelEvent(Panel * panel, const SDL_Event * event);
void RenderPanelTexture(const Panel * panel);
void PanelPrint(const Panel * panel, int x, int y, const char * string);
void UpdatePanel(const Panel * panel, int x, int y, u8 ch, bool setTarget);

/// Render a gray bar on the panel's current selection.
void RenderPanelSelection(const Panel * panel);

SDL_Rect PanelRenderLocation(const Panel * panel);
void SetPanelColor(int index);
void RenderMark(const PanelItem * item, int value);
void RenderPanelTextInput(Panel * panel);

/// Caller should fill panel->text prior to calling.
void StartTextEditing(Panel * panel, int itemIndex, int * value);

#endif /* panel_h */
