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

#define PANEL_DATA_DIRECTORY "panel_data/"
#define MAX_PANELS 10
#define MAX_TEXT 256
#define PANEL_SCREEN_MARGIN 16
#define PALETTE_ITEM_MARGIN 16

#define SELECTION_BOX_MARGIN 5
#define SELECTION_BOX_THICKNESS 2
#define SELECTION_BOX_COLOR 4 // Dark Red

#define PANEL_RENDER_STRING(x, y, ...) \
    RenderString((x) * FONT_WIDTH, (y) * FONT_HEIGHT, __VA_ARGS__)

// TODO: type? i.e. radio, toggle, input
typedef struct
{
    int x, y;   // Location within Panel (relative)
    int width;  // Selection highlight width, proceeding horizontally fron x.

    // Rectangular area for mouse selection.
    bool hasMouseRect; // If false, just use x, y and width for highlight.
    int mouseX1;
    int mouseY1;
    int mouseX2;
    int mouseY2;
} PanelItem;


// TODO: if text input, WASD shouldn't scroll map
typedef struct panel
{
    u8 width;  // in text columns
    u8 height; // text rows

    // In window space, or relative to this panel's parent,
    // if `parent` is non-NULL.
    SDL_Rect location;

    // TODO: probably unneeded, remove
    struct panel * parent; // Reference to the panel that opened this panel.

    bool dynamicItems; // `items` has been dynamically allocated.

    // Reference to this panel's items array, or allocated
    // memory if `dynamicItems`.
    PanelItem * items;
    int numItems;

    int selection; // Index into `items`

    SDL_Point mouseLocation; // Relative to the panel origin.
    SDL_Point textLocation; // Mouse location in text console coordinates.

    // The allocated "console" buffer and the SDL_Texture that displays it.
    SDL_Texture * texture;
    u16 * consoleData;

    // For the panel's text editing state.
    bool isTextEditing;
    enum { VALUE_INT, VALUE_STRING } valueType;
    char text[MAX_TEXT]; // The text being display and worked on while editing.
    void * value; // A reference to the value that will be commited.
    int cursor;
    int textItem; // The item where text is current being editing.

    bool (* processEvent)(const SDL_Event *);
    void (* render)(void);
    void (* textEditingCompletionHandler)(void);
} Panel;


void LoadPanelConsole(Panel * panel, const char * path);
Panel NewPanel(int x, int y, int width, int height, int numItems);
void FreePanel(const Panel * panel);
bool ProcessPanelEvent(Panel * panel, const SDL_Event * event);
void RenderPanelTexture(const Panel * panel);
void RenderPanel(const Panel * panel);


void SetPanelColor(int text, int background);

/// Update the panel's console buffer at `x`, `y`.
void ConsolePrint(const Panel * panel, int x, int y, const char * string);

/// Write `ch` to panel console at `x`, `y` and update texture.
void UpdatePanelConsole(const Panel * panel, int x, int y, u8 ch);

/// Render a gray bar on the panel's current selection.
void RenderPanelSelection(const Panel * panel);

SDL_Rect PanelRenderLocation(const Panel * panel);
void SetPanelRenderColor(int index);
void RenderMark(const PanelItem * item, int value);
void RenderPanelTextInput(const Panel * panel);
bool ShouldRenderInactiveTextField(const Panel * panel, int itemIndex);

/// Caller should fill panel->text prior to calling.
void StartTextEditing(Panel * panel, int itemIndex, void * value, int type);

bool IsMouseActionEvent(const SDL_Event * event, const Panel * panel);
bool DidClickOnItem(const SDL_Event * event, const Panel * panel);
void UpdatePanelMouse(const SDL_Point * windowMouse);

#pragma mark - SCROLLBAR

typedef struct {
    enum { SCROLLBAR_HORIZONTAL, SCROLLBAR_VERTICAL } type;
    int location; // The row or col of the scroll bar.

    // The span of the scroll bar. (x if horizontal, y if vertical)
    int max; // in text coordinates
    int min;

    bool isDragging;

    int scrollPosition; // The y position visible at the top of the panel.
    int maxScrollPosition; // The maximum y position that can be scrolled to.
} Scrollbar;

/// x, y is (usually) the click point in console coordinates
/// - returns: the position or -1 if x, y is outside the scrollbar.
int GetPositionInScrollbar(const Scrollbar * scrollbar, int x, int y);
void ScrollToPosition(Scrollbar * scrollbar, int position);
int GetScrollbarHandlePosition(Scrollbar * scrollbar, int textPosition);

#endif /* panel_h */
