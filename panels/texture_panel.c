//
//  texture_panel.c
//  de
//
//  Created by Thomas Foster on 6/16/23.
//

#include "texture_panel.h"

#include "patch.h"
#include "line_panel.h"
#include "text.h"

#define PALETTE_WIDTH 272
#define SCROLL_BAR_COL 40
#define SCROLL_BAR_TOP 2
#define SCROLL_BAR_BOTTOM 36

Panel texturePanel;
SDL_Rect paletteRectRelative;
int paletteTopY = 0;
int maxPaletteTopY; // Calculated during init.
bool draggingScrollHandle;
char * currentTexture;

typedef struct
{
    int width;
    int height;
    char name[9];
} Filter;

static Filter filter = {
    .width = 0,
    .height = 0,
//    .name = "door"
};

enum
{
    TP_NAME,
    TP_WIDTH,
    TP_HEIGHT,
    TP_COUNT,
};

static PanelItem items[] =
{
    [TP_NAME] = { 8, 39, 8, -1, -1, -1, TP_WIDTH, true, 2, 39, 15, 39 },
    [TP_WIDTH] = { 24, 39, 4, -1, -1, TP_NAME, TP_HEIGHT, true, 17, 39, 27, 39 },
    [TP_HEIGHT] = { 37, 39, 4, -1, -1, TP_WIDTH, -1, true, 29, 39, 40, 39 },
};

SDL_Rect PaletteRect(void)
{
    SDL_Rect rect;
    rect.x = texturePanel.location.x + paletteRectRelative.x;
    rect.y = texturePanel.location.y + paletteRectRelative.y;
    rect.w = paletteRectRelative.w;
    rect.h = paletteRectRelative.h;

    return rect;
}

static void ScrollToSelected(void)
{
    if ( currentTexture[0] == '-' )
        paletteTopY = 0;
    else
    {
        Texture * texture = resourceTextures->data;
        for ( int i = 0; i < resourceTextures->count; i++, texture++ )
        {
            if ( strcmp(texture->name, currentTexture) == 0 )
                paletteTopY = texture->rect.y - PALETTE_ITEM_MARGIN;
        }
    }
}

void OpenTexturePanel(char * texture)
{
    currentTexture = texture;
    rightPanels[++topPanel] = &texturePanel;

    ScrollToSelected();
}

static bool IsFilteredOut(Texture * texture)
{
    if ( filter.width > 0 && texture->rect.w != filter.width )
        return true;

    if ( filter.height > 0 && texture->rect.h != filter.height )
        return true;

    if ( filter.name[0] != '\0' && strcasestr(texture->name, filter.name) == NULL )
        return true;

    return false;
}

static void UpdatePaletteTextureLocations(void)
{
    Texture * texture = resourceTextures->data;
    int x = PALETTE_ITEM_MARGIN;
    int y = PALETTE_ITEM_MARGIN;
    int maxRowHeight = 0;
    int maxTextureY = 0;

    for ( int i = 0; i < resourceTextures->count; i++, texture++ )
    {
        if ( IsFilteredOut(texture) )
            continue;

        if ( x + texture->rect.w > paletteRectRelative.w - PALETTE_ITEM_MARGIN )
        {
            x = PALETTE_ITEM_MARGIN;
            y += maxRowHeight + PALETTE_ITEM_MARGIN;
            maxRowHeight = 0;
        }

        if ( texture->rect.h > maxRowHeight )
            maxRowHeight = texture->rect.h;

        texture->rect.x = x;
        texture->rect.y = y;

        x += texture->rect.w + PALETTE_ITEM_MARGIN;

        maxTextureY = texture->rect.y + maxRowHeight;
    }

    maxPaletteTopY = maxTextureY + PALETTE_ITEM_MARGIN - paletteRectRelative.h;
//    printf("max palette top y: %d\n", maxPaletteTopY);
}

void SetTopYFromScrollBar(void)
{
    float percent = (float)(texturePanel.textLocation.y - SCROLL_BAR_TOP) / (SCROLL_BAR_BOTTOM - SCROLL_BAR_TOP);
    paletteTopY = maxPaletteTopY * percent;

    CLAMP(paletteTopY, 0, maxPaletteTopY);
}

static void FinishTextEditing(void)
{
    UpdatePaletteTextureLocations();
    paletteTopY = 0;
}

#pragma mark -

void DoTexturePanelAction(void)
{
    switch ( texturePanel.selection )
    {
        case TP_NAME:
            StartTextEditing(&texturePanel,
                             texturePanel.selection,
                             filter.name,
                             VALUE_STRING);
            break;
        case TP_WIDTH:
            StartTextEditing(&texturePanel,
                             texturePanel.selection,
                             &filter.width,
                             VALUE_INT);
            break;
        case TP_HEIGHT:
            StartTextEditing(&texturePanel,
                             texturePanel.selection,
                             &filter.height,
                             VALUE_INT);
            break;
        default:
            break;
    }
}


bool ProcessTexturePanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event, &texturePanel) )
    {
        DoTexturePanelAction();
        return true;
    }

    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_BACKSPACE:
                    if ( texturePanel.selection == TP_WIDTH )
                        filter.width = 0;
                    else if ( texturePanel.selection == TP_HEIGHT )
                        filter.height = 0;
                    else if ( texturePanel.selection == TP_NAME )
                        filter.name[0] = '\0';
                    UpdatePaletteTextureLocations();
                    return true;
                default:
                    return false;
            }
            break;
        case SDL_MOUSEWHEEL:
            paletteTopY += event->wheel.y * 16;
            CLAMP(paletteTopY, 0, maxPaletteTopY);
            return true;

        case SDL_MOUSEMOTION:
            if ( draggingScrollHandle ) {
                SetTopYFromScrollBar();
                return true;
            }
            return false;

        case SDL_MOUSEBUTTONDOWN:
        {
            if ( event->button.button == SDL_BUTTON_LEFT )
            {
                if (   texturePanel.textLocation.x == SCROLL_BAR_COL
                    && texturePanel.textLocation.y >= SCROLL_BAR_TOP
                    && texturePanel.textLocation.y <= SCROLL_BAR_BOTTOM )
                {
                    draggingScrollHandle = true;
                    SetTopYFromScrollBar();
                }
                else if ( SDL_PointInRect(&texturePanel.mouseLocation,
                                          &paletteRectRelative) )
                {
                    SDL_Point location = texturePanel.mouseLocation;

                    // Convert to paletteRect coordinate space.
                    location.x -= paletteRectRelative.x;
                    location.y -= paletteRectRelative.y;
                    location.y += paletteTopY;

                    Texture * t = resourceTextures->data;
                    for ( int i = 0; i < resourceTextures->count; i++, t++ )
                    {
                        if ( IsFilteredOut(t) )
                            continue;

                        if ( SDL_PointInRect(&location, &t->rect) )
                        {
                            strncpy(currentTexture, t->name, 8);
                            UpdateLinePanelContent();
                            if ( event->button.clicks == 2 )
                                topPanel--;
                        }
                    }
                }
                return true;
            }
            return false;
        }

        case SDL_MOUSEBUTTONUP:
            if ( event->button.button == SDL_BUTTON_LEFT && draggingScrollHandle )
            {
                draggingScrollHandle = false;
                return true;
            }
            return false;
        default:
            return false;
    }
}

void RenderTexturePanel(void)
{
    // Texture palette

    SDL_Rect paletteRect = PaletteRect();
    SDL_RenderSetViewport(renderer, &paletteRect);

    Texture * texture = resourceTextures->data;
    int top = paletteTopY;
    int bottom = top + paletteRect.h;

    for ( int i = 0; i < resourceTextures->count; i++, texture++ )
    {
        if ( IsFilteredOut(texture) )
            continue;

        if (   texture->rect.y + texture->rect.h >= top
            && texture->rect.y < bottom )
        {
            RenderTexture(texture,
                          texture->rect.x,
                          texture->rect.y - paletteTopY,
                          1.0f);

            if ( strcmp(currentTexture, texture->name) == 0 )
            {
                SDL_Rect box = texture->rect;
                box.y -= paletteTopY;
                
                int margin = SELECTION_BOX_MARGIN;
                box.x -= margin;
                box.y -= margin;
                box.w += margin * 2;
                box.h += margin * 2;

                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                DrawRect(&(SDL_FRect){ box.x, box.y, box.w, box.h },
                         SELECTION_BOX_THICKNESS);
            }
        }
    }

    SDL_RenderSetViewport(renderer, &texturePanel.location);

    // TODO: clean this up
    if ( !texturePanel.isTextEditing || (texturePanel.isTextEditing && texturePanel.textItem != TP_WIDTH) )
    {
        if ( filter.width <= 0 )
        {
            SetPanelRenderColor(8);
            PANEL_RENDER_STRING(items[TP_WIDTH].x, items[TP_WIDTH].y, "Any");
        }
        else
        {
            SetPanelRenderColor(15);
            PANEL_RENDER_STRING(items[TP_WIDTH].x, items[TP_WIDTH].y, "%d", filter.width);
        }
    }

    if ( !texturePanel.isTextEditing || (texturePanel.isTextEditing && texturePanel.textItem != TP_HEIGHT) )
    {
        if ( filter.height <= 0 )
        {
            SetPanelRenderColor(8);
            PANEL_RENDER_STRING(items[TP_HEIGHT].x, items[TP_HEIGHT].y, "Any");
        }
        else
        {
            SetPanelRenderColor(15);
            PANEL_RENDER_STRING(items[TP_HEIGHT].x, items[TP_HEIGHT].y, "%d", filter.height);
        }
    }

    if ( !texturePanel.isTextEditing || (texturePanel.isTextEditing && texturePanel.textItem != TP_NAME) )
    {
        SetPanelRenderColor(15);
        PANEL_RENDER_STRING(items[TP_NAME].x, items[TP_NAME].y, "%s", filter.name);
    }

    // Scroll bar control

    int x = SCROLL_BAR_COL * FONT_WIDTH;
    float percent = (float)paletteTopY / maxPaletteTopY;
    int barHeight = (SCROLL_BAR_BOTTOM - SCROLL_BAR_TOP) * FONT_HEIGHT;
    int y = (SCROLL_BAR_TOP * FONT_HEIGHT) + barHeight * percent;
    SetPanelRenderColor(15);
    RenderChar(x, y, 8);

    // The border around the texture palette (in window space)

    SDL_RenderSetViewport(renderer, NULL);
    SetPanelRenderColor(8);
    SDL_RenderDrawRect(renderer, &paletteRect);
}

void LoadTexturePanel(void)
{
    texturePanel = LoadPanel(PANEL_DATA_DIRECTORY"texture.panel");
    texturePanel.location.y = 0;
    texturePanel.render = RenderTexturePanel;
    texturePanel.processEvent = ProcessTexturePanelEvent;
    texturePanel.textEditingCompletionHandler = FinishTextEditing;
    texturePanel.items = items;
    texturePanel.numItems = TP_COUNT;

    paletteRectRelative = (SDL_Rect)
    {
        .x = 2 * FONT_WIDTH,
        .y = 1 * FONT_HEIGHT,
        .w = 38 * FONT_WIDTH,
        .h = 37 * FONT_HEIGHT
    };

    UpdatePaletteTextureLocations();
}
