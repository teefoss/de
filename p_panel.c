//
//  panel.c
//  de
//
//  Created by Thomas Foster on 5/27/23.
//

#include "p_panel.h"
#include "common.h"
#include "text.h"
#include "e_editor.h"
#include "e_defaults.h"

#include <string.h>

Panel * panelStack[MAX_PANELS];
int topPanel = -1;
int mousePanel = -1; // Panel mouse is currently over.

int textColor = 15;
int backgroundColor = 1;

void SetTextColor(int index)
{
    textColor = index;
}

void SetBackgroundColor(int index)
{
    backgroundColor = index;
}

void SetPanelColor(int text, int background)
{
    textColor = text;
    backgroundColor = background;
}

void SetPanelRenderColor(int index)
{
    SDL_Color c = Int24ToSDL(palette[index]);

    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
}

void OpenPanel(Panel * panel, void * data)
{
    panelStack[++topPanel] = panel;

    // Place it in the middle of the editor window.
    SDL_Rect windowFrame = GetWindowFrame();
    panel->location.x = (windowFrame.w - panel->location.w) / 2;
    panel->location.y = (windowFrame.h - panel->location.h) / 2;

    panel->data = data;
}

void UpdatePanelConsole(const Panel * panel, int x, int y, u8 ch, bool setTarget)
{
    if ( setTarget )
        SDL_SetRenderTarget(renderer, panel->texture);

    int i = y * panel->width + x;
    u8 attr = (backgroundColor << 4) | textColor;
    panel->consoleData[i] = (attr << 8) | ch;

    static SDL_Rect r = { .w = FONT_WIDTH, .h = FONT_HEIGHT };
    r.x = x * FONT_WIDTH;
    r.y = y * FONT_HEIGHT;

    SetPanelRenderColor(backgroundColor);
    SDL_RenderFillRect(renderer, &r);

    SetPanelRenderColor(textColor);
    RenderChar(r.x, r.y, ch);

    if ( setTarget )
        SDL_SetRenderTarget(renderer, NULL);
}

void PanelPrint(const Panel * panel, int x, int y, const char * string)
{
    SDL_SetRenderTarget(renderer, panel->texture);

    const char * c = string;
    while ( *c != '\0' )
    {
        UpdatePanelConsole(panel, x++, y, *c, false);
        c++;
    }

    SDL_SetRenderTarget(renderer, NULL);
}

Panel NewPanel(int x, int y, int width, int height, int numItems)
{
    Panel panel = { 0 };

    panel.location.x = x;
    panel.location.y = y;
    panel.location.w = width * FONT_WIDTH;
    panel.location.h = height * FONT_HEIGHT;

    panel.width = width;
    panel.height = height;

    panel.consoleData = calloc(width * height, sizeof(*panel.consoleData));

    panel.texture = SDL_CreateTexture(renderer,
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET,
                                      panel.location.w,
                                      panel.location.h);

    panel.numItems = numItems;
    panel.items = calloc(numItems, sizeof(*panel.items));

    return panel;
}

void FreePanel(const Panel * panel)
{
    if ( panel->dynamicItems )
        free(panel->items);

    free(panel->consoleData);
    SDL_DestroyTexture(panel->texture);
}

void LoadPanelConsole(Panel * panel, const char * path)
{
    FILE * file = fopen(path, "rb");
    if ( file == NULL )
        Error("Error: '%s' does not exist\n", path);

    u8 width;
    u8 height;
    fread(&width, sizeof(width), 1, file);
    fread(&height, sizeof(height), 1, file);

    u16 * data = malloc(sizeof(*data) * width * height);
    fread(data, sizeof(*data), width * height, file);
    panel->consoleData = data;

    fclose(file);

    SDL_Texture * texture = SDL_CreateTexture(renderer,
                                              SDL_PIXELFORMAT_RGBA8888,
                                              SDL_TEXTUREACCESS_TARGET,
                                              width * FONT_WIDTH,
                                              height * FONT_HEIGHT);

    if ( texture == NULL )
        Error("Error: could not load line panel (%s)\n", SDL_GetError());

    SDL_SetRenderTarget(renderer, texture);

    for ( int y = 0; y < height; y++ )
    {
        for ( int x = 0; x < width; x++ )
        {
            BufferCell cell = GetCell(data[y * width + x]);
            textColor = cell.foreground;
            backgroundColor = cell.background;
            UpdatePanelConsole(panel, x, y, cell.character, false);
        }
    }

    SDL_SetRenderTarget(renderer, NULL);

    panel->width = width;
    panel->height = height;
    panel->location.w = width * FONT_WIDTH;
    panel->location.h = height * FONT_HEIGHT;
    panel->texture = texture;
}

SDL_Rect PanelRenderLocation(const Panel * panel)
{
    SDL_Rect rect = panel->location;

    if ( panel->parent ) {
        rect.x += panel->parent->location.x;
        rect.y += panel->parent->location.y;
    }

    return rect;
}

void RenderMark(const PanelItem * item, int value)
{
    if ( value != 0 ) {
        SetPanelRenderColor(15);
        RenderChar((item->x - 3) * FONT_WIDTH, item->y * FONT_HEIGHT, 7);
    }
}

void RenderPanelTexture(const Panel * panel)
{
    SDL_Rect dest = PanelRenderLocation(panel);

    // Shadow
    SDL_Rect shadow = dest;
    shadow.x += 16;
    shadow.y += 16;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(renderer, &shadow);

    // Texture
    SDL_RenderCopy(renderer, panel->texture, NULL, &dest);

    // Outline
    SetPanelRenderColor(11);
    SDL_RenderDrawRect(renderer, &dest);
}

/// Render a colored horizontal bar at panel item by extracting the console
/// information under the bar, and rerendering it on top with the given color.
void RenderBar(const Panel * panel, const PanelItem * item, int color)
{
    for ( int x = item->x; x < item->x + item->width; x++ )
    {
        int y = item->y;
        int i = y * panel->width + x;
        BufferCell cell = GetCell(panel->consoleData[i]);

        SDL_Rect r = {
            x * FONT_WIDTH,
            item->y * FONT_HEIGHT,
            FONT_WIDTH,
            FONT_HEIGHT
        };

        SetPanelRenderColor(color);
        SDL_RenderFillRect(renderer, &r);

        SetPanelRenderColor(15);
        RenderChar(x * FONT_WIDTH, y * FONT_HEIGHT, cell.character);
    }
}

void RenderPanelSelection(const Panel * panel)
{
    if ( panel->items == NULL )
        return;

    SDL_Rect oldViewport;
    SDL_RenderGetViewport(renderer, &oldViewport); // Save current viewport.
    SDL_Rect renderLocation = PanelRenderLocation(panel);
    SDL_RenderSetViewport(renderer, &renderLocation);

    // Only render selection if the mouse is actually over an item.
    if ( panel->selection != -1 )
        RenderBar(panel, &panel->items[panel->selection], 7);

    SDL_RenderSetViewport(renderer, &oldViewport); // Restore viewport.
}

void RenderPanelTextInput(const Panel * panel)
{
    SetPanelRenderColor(15);
    int x = panel->items[panel->textItem].x;
    int y = panel->items[panel->textItem].y;
    PANEL_RENDER_STRING(x, y, panel->text);

    if ( SDL_GetTicks() % 600 < 300 )
        PANEL_RENDER_STRING(x + panel->cursor, y, "_");
}

bool ShouldRenderInactiveTextField(const Panel * panel, int itemIndex)
{
    return !panel->isTextEditing || (panel->isTextEditing && panel->textItem != itemIndex);
}

void RenderPanel(const Panel * panel)
{
    // Texture

    RenderPanelTexture(panel);

    // Text input / selection / etc.

    SDL_RenderSetViewport(renderer, &panel->location);

    if ( panel->isTextEditing )
        RenderPanelTextInput(panel);
    else if ( panel->selection != -1 )
        RenderPanelSelection(panel);

    // Per-panel rendering?

    if ( panel->render )
        panel->render();

    SDL_RenderSetViewport(renderer, NULL);
}

// TODO: confirm that itemIndex is not necessary (use panel->selection?)
void StartTextEditing(Panel * panel, int itemIndex, void * value, int type)
{
//    if ( type == VALUE_INT )
//        SDL_itoa(*(int *)value, panel->text, 10);
//    else if ( type == VALUE_STRING )
//        strncpy(panel->text, (char *)value, sizeof(panel->text));
    memset(panel->text, 0, sizeof(panel->text));
    panel->text[0] = '\0';
    panel->cursor = 0;

    panel->isTextEditing = true;
    panel->valueType = type;
    panel->textItem = itemIndex;
//    panel->cursor = (int)strnlen(panel->text, MAX_TEXT);
    panel->value = value;

    SDL_StartTextInput();
}

void StopTextEditing(Panel * panel)
{
    panel->isTextEditing = false;
    SDL_StopTextInput();
}

void ProcessPanelTextInput(Panel * panel, const SDL_Event * event)
{
    int len = (int)strlen(panel->text);

    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_RETURN:
                    if ( panel->valueType == VALUE_INT )
                    {
                        // Convert `text` back to an int and set value if valid.
                        char * end;
                        long value = strtol(panel->text, &end, 10);
                        if ( *end == '\0' )
                        *(int *)panel->value = (int)value;
                    }
                    else if ( panel->valueType == VALUE_STRING )
                    {
                        strcpy((char *)panel->value, panel->text);
                    }

                    StopTextEditing(panel);
                    if ( panel->textEditingCompletionHandler )
                        panel->textEditingCompletionHandler();
                    break;
                case SDLK_ESCAPE:
                    StopTextEditing(panel);
                    break;
                case SDLK_BACKSPACE:
                    if ( panel->cursor >= 1 )
                    {
                        for ( int x = panel->cursor; x <= len; x++ )
                            panel->text[x - 1] = panel->text[x];
                        panel->cursor--;
                    }
                    break;
                case SDLK_LEFT:
                    if ( panel->cursor >= 0 )
                        panel->cursor--;
                    break;
                case SDLK_RIGHT:
                    if ( panel->cursor < len )
                        panel->cursor++;
                    break;
                default:
                    break;
            }
            break;

        case SDL_TEXTINPUT:
            for ( int x = len; x > panel->cursor; x-- )
                panel->text[x] = panel->text[x - 1];
            panel->text[panel->cursor] = event->text.text[0];
            panel->cursor++;
            break;

        default:
            break;
    }
}

bool ProcessPanelEvent(Panel * panel, const SDL_Event * event)
{
    if ( panel->isTextEditing )
    {
        ProcessPanelTextInput(panel, event);
        return true;
    }

    if ( panel->processEvent && panel->processEvent(event) )
        return true;

    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_ESCAPE: // Close panel.
                    if ( topPanel >= 0 )
                        topPanel--;
                    return true;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    return false;
}

int GetPanelStackPosition(const Panel * panel)
{
    for ( int i = 0; i <= topPanel; i++ )
        if ( panelStack[i] == panel )
            return i;

    return -1;
}

bool IsMouseActionEvent(const SDL_Event * event, const Panel * panel)
{
    return event->type == SDL_MOUSEBUTTONDOWN
    && event->button.button == SDL_BUTTON_LEFT
    && panel->selection != -1;
}

bool IsActionEvent(const SDL_Event * event, const Panel * panel)
{
    return IsMouseActionEvent(event, panel);
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

int GetPositionInScrollbar(const Scrollbar * scrollbar, int x, int y)
{
    bool aligned;
    bool inside;

    if ( scrollbar->type == SCROLLBAR_VERTICAL )
    {
        aligned = x == scrollbar->location;
        inside = y >= scrollbar->min && y <= scrollbar->max;

        if ( aligned && inside )
            return y - scrollbar->min;
    }
    else
    {
        aligned = y == scrollbar->location;
        inside = x >= scrollbar->min && x <= scrollbar->max;

        if ( aligned && inside )
            return x - scrollbar->min;
    }

    return -1;
}

void ScrollToPosition(Scrollbar * scrollbar, int position)
{
    int height = scrollbar->max - scrollbar->min;
    float percent = (float)(position - scrollbar->min) / height;
    scrollbar->scrollPosition = scrollbar->maxScrollPosition * percent;

    CLAMP(scrollbar->scrollPosition, 0, scrollbar->maxScrollPosition);
}
