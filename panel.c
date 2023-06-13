//
//  panel.c
//  de
//
//  Created by Thomas Foster on 5/27/23.
//

#include "panel.h"
#include "common.h"
#include "text.h"

#include <string.h>

Panel * openPanels[MAX_PANELS];
int topPanel = -1;

int textColor = 15;
int backgroundColor = 1;

static SDL_Color palette[16] = {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xAA },
//    { 0x04, 0x14, 0x41 }, // Get that funkly blue color!
    { 0x00, 0xAA, 0x00 },
    { 0x00, 0xAA, 0xAA },
    { 0xAA, 0x00, 0x00 },
    { 0xAA, 0x00, 0xAA },
    { 0xAA, 0x55, 0x00 },
    { 0xAA, 0xAA, 0xAA },
    { 0x55, 0x55, 0x55 },
    { 0x55, 0x55, 0xFF },
    { 0x55, 0xFF, 0x55 },
    { 0x55, 0xFF, 0xFF },
    { 0xFF, 0x55, 0x55 },
    { 0xFF, 0x55, 0xFF },
    { 0xFF, 0xFF, 0x55 },
    { 0xFF, 0xFF, 0xFF },
};


void SetPanelColor(int index)
{
    SDL_Color c = palette[index];

    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
}

void UpdatePanel(const Panel * panel, int x, int y, u8 ch, bool setTarget)
{
    if ( setTarget )
        SDL_SetRenderTarget(renderer, panel->texture);

    int i = y * panel->width + x;
    u8 attr = (backgroundColor << 4) | textColor;
    panel->consoleData[i] = (attr << 8) | ch;

    static SDL_Rect r = { .w = FONT_WIDTH, .h = FONT_HEIGHT };
    r.x = x * FONT_WIDTH;
    r.y = y * FONT_HEIGHT;

    SetPanelColor(backgroundColor);
    SDL_RenderFillRect(renderer, &r);

    SetPanelColor(textColor);
    PrintChar(r.x, r.y, ch);

    if ( setTarget )
        SDL_SetRenderTarget(renderer, NULL);
}

void PanelPrint(const Panel * panel, int x, int y, const char * string)
{
    SDL_SetRenderTarget(renderer, panel->texture);

    const char * c = string;
    while ( *c != '\0' )
    {
        UpdatePanel(panel, x++, y, *c, false);
        c++;
    }

    SDL_SetRenderTarget(renderer, NULL);
}

Panel LoadPanel(const char * path)
{
    Panel panel = { 0 };

    FILE * file = fopen(path, "rb");
    if ( file == NULL ) {
        printf("Error: '%s' does not exist\n", path);
        return panel;
    }

    u8 width;
    u8 height;
    fread(&width, sizeof(width), 1, file);
    fread(&height, sizeof(height), 1, file);

    u16 * data = malloc(sizeof(*data) * width * height);
    fread(data, sizeof(*data), width * height, file);
    panel.consoleData = data;

    fclose(file);

    SDL_Texture * texture = SDL_CreateTexture(renderer,
                                              SDL_PIXELFORMAT_RGBA8888,
                                              SDL_TEXTUREACCESS_TARGET,
                                              width * FONT_WIDTH,
                                              height * FONT_HEIGHT);

    if ( texture == NULL )
    {
        printf("Error: could not load line panel (%s)\n", SDL_GetError());
        return panel;
    }

    SDL_SetRenderTarget(renderer, texture);

    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            BufferCell cell = GetCell(data[y * width + x]);
            textColor = cell.foreground;
            backgroundColor = cell.background;
            UpdatePanel(&panel, x, y, cell.character, false);
        }
    }

    SDL_SetRenderTarget(renderer, NULL);

    panel.width = width;
    panel.height = height;
    panel.location.w = width * FONT_WIDTH;
    panel.location.h = height * FONT_HEIGHT;
    panel.texture = texture;

    return panel;
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
        SetPanelColor(15);
        PrintChar((item->x - 3) * FONT_WIDTH, item->y * FONT_HEIGHT, 7);
    }
}

void RenderPanelTexture(const Panel * panel)
{
    SDL_Rect dest = PanelRenderLocation(panel);

    SDL_Rect shadow = dest;
    shadow.x += 16;
    shadow.y += 16;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_RenderFillRect(renderer, &shadow);
    SDL_RenderCopy(renderer, panel->texture, NULL, &dest);
}

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

        SetPanelColor(color);
        SDL_RenderFillRect(renderer, &r);

        SetPanelColor(15);
        PrintChar(x * FONT_WIDTH, y * FONT_HEIGHT, cell.character);
    }
}

void RenderPanelSelection(const Panel * panel)
{
    SDL_Rect oldViewport;
    SDL_RenderGetViewport(renderer, &oldViewport); // Save current viewport.
    SDL_Rect renderLocation = PanelRenderLocation(panel);
    SDL_RenderSetViewport(renderer, &renderLocation);

    RenderBar(panel, &panel->items[panel->selection], 7);

    if ( panel->mouseHover != -1 )
        RenderBar(panel, &panel->items[panel->mouseHover], 3);

    SDL_RenderSetViewport(renderer, &oldViewport); // Restore viewport.
}

void RenderPanelTextInput(Panel * panel)
{
    int x = panel->items[panel->textItem].x;
    int y = panel->items[panel->textItem].y;
    PANEL_PRINT(x, y, panel->text);

    if ( SDL_GetTicks() % 600 < 300 )
        PANEL_PRINT(x + panel->cursor, y, "_");
}

void StartTextEditing(Panel * panel, int itemIndex, int * value)
{
    panel->isTextEditing = true;
    panel->textItem = itemIndex;
    panel->cursor = (int)strnlen(panel->text, MAX_TEXT);
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
                    *panel->value = (int)strtol(panel->text, NULL, 10);
                    StopTextEditing(panel);
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

    // Change selection:

    if ( panel->items == NULL )
        return false;

    PanelItem * item = &panel->items[panel->selection];

    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_UP:
                    if ( item->up == 0 )
                        panel->selection--;
                    else if ( item->up > 0 )
                        panel->selection = item->up;
                    return true;

                case SDLK_DOWN:
                    if ( item->down == 0
                        && panel->selection < panel->numItems - 1 )
                        panel->selection++;
                    else if ( item->down > 0 )
                        panel->selection = item->down;
                    return true;

                case SDLK_LEFT:
                    if ( item->left != -1 )
                        panel->selection = item->left;
                    return true;

                case SDLK_RIGHT:
                    if ( item->right != -1 )
                        panel->selection = item->right;
                    return true;

                default:
                    return false;
            }

        default:
            return false;
    }
}
