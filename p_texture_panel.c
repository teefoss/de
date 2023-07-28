//
//  texture_panel.c
//  de
//
//  Created by Thomas Foster on 6/16/23.
//

#include "p_texture_panel.h"

#include "g_patch.h"
#include "p_stack.h"
#include "p_line_panel.h"
#include "text.h"

#define PALETTE_WIDTH 272

// TODO: convert to use Scrollbar
#define SCROLL_BAR_COL 40
#define SCROLL_BAR_TOP 3
#define SCROLL_BAR_BOTTOM 37

Panel texturePanel;
SDL_Rect paletteRectRelative;
int paletteTopY = 0;
int maxPaletteTopY; // Calculated during init.
bool draggingScrollHandle;

char * currentTexture;

// Size of selected texture
int currentWidth;
int currentHeight;
LineProperty texturePosition; // Whether we're selecting top/middle/bottom

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
    TP_SIZE,
    TP_REMOVE,
    TP_FILTER_NAME,
    TP_FILTER_WIDTH,
    TP_FILTER_HEIGHT,
    TP_COUNT,
};

static PanelItem items[] =
{
    [TP_NAME] = { 8, 39, 8, true, 2, 38, 16, 39 },
    [TP_SIZE] = { 24, 39, 0 },
    [TP_REMOVE] = { 35, 39, 6 },
    [TP_FILTER_NAME] = { 8, 41, 8, true, 2, 41, 15, 41 },
    [TP_FILTER_WIDTH] = { 24, 41, 4, true, 17, 41, 27, 41 },
    [TP_FILTER_HEIGHT] = { 37, 41, 4, true, 29, 41, 40, 41 },
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

static bool IsFilteredOut(Texture * texture)
{
    if ( filter.width > 0 && texture->rect.w != filter.width )
        return true;

    if ( filter.height > 0 && texture->rect.h != filter.height )
        return true;

    if ( filter.name[0] != '\0'
        && strcasestr(texture->name, filter.name) == NULL )
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


void OpenTexturePanel(char * texture, LineProperty property)
{
    currentTexture = texture;
    texturePosition = property;

    Texture * t = FindTexture(texture);
    if ( t )
    {
        currentWidth = t->rect.w;
        currentHeight = t->rect.h;
    }

    OpenPanel(&texturePanel);

    ScrollToSelected();

    filter.width = 0;
    filter.height = 0;
    filter.name[0] = '\0';

    UpdatePaletteTextureLocations();
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
            ScrollToSelected();
            break;
        case TP_FILTER_NAME:
            StartTextEditing(&texturePanel,
                             texturePanel.selection,
                             filter.name,
                             VALUE_STRING);
            break;
        case TP_FILTER_WIDTH:
            StartTextEditing(&texturePanel,
                             texturePanel.selection,
                             &filter.width,
                             VALUE_INT);
            break;
        case TP_FILTER_HEIGHT:
            StartTextEditing(&texturePanel,
                             texturePanel.selection,
                             &filter.height,
                             VALUE_INT);
            break;

        case TP_REMOVE:
            strcpy(currentTexture, "-");
            CloseTopPanel();
            UpdateLinePanelContent();
            LinePanelApplyChange(texturePosition);
            break;

        default:
            break;
    }
}


bool ProcessTexturePanelEvent(const SDL_Event * event)
{
    if ( DidClickOnItem(event, &texturePanel) )
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
                    if ( texturePanel.selection == TP_FILTER_WIDTH )
                        filter.width = 0;
                    else if ( texturePanel.selection == TP_FILTER_HEIGHT )
                        filter.height = 0;
                    else if ( texturePanel.selection == TP_NAME )
                        filter.name[0] = '\0';
                    UpdatePaletteTextureLocations();
                    return true;
                case SDLK_n:
                    texturePanel.selection = TP_FILTER_NAME;
                    DoTexturePanelAction();
                    return true;
                case SDLK_w:
                    texturePanel.selection = TP_FILTER_WIDTH;
                    DoTexturePanelAction();
                    return true;
                case SDLK_h:
                    texturePanel.selection = TP_FILTER_HEIGHT;
                    DoTexturePanelAction();
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
                            currentWidth = t->rect.w;
                            currentHeight = t->rect.h;

                            LinePanelApplyChange(texturePosition);
                            UpdateLinePanelContent();

                            if ( event->button.clicks == 2 )
                                CloseTopPanel();
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
    // Title

    SetPanelRenderColor(15);
    int titleX = 2;
    int titleY = 1;
    if ( texturePosition == LINE_TOP_TEXTURE )
        PANEL_RENDER_STRING(titleX, titleY, "Top Texture");

    if ( texturePosition == LINE_MIDDLE_TEXTURE )
        PANEL_RENDER_STRING(titleX, titleY, "Middle Texture");

    if ( texturePosition == LINE_BOTTOM_TEXTURE )
        PANEL_RENDER_STRING(titleX, titleY, "Bottom Texture");

    //
    // Texture palette
    //

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
    if ( ShouldRenderInactiveTextField(&texturePanel, TP_FILTER_WIDTH) )
    {
        // TODO: refactor this with below
        if ( filter.width <= 0 )
        {
            SetPanelRenderColor(8);
            PANEL_RENDER_STRING(items[TP_FILTER_WIDTH].x, items[TP_FILTER_WIDTH].y, "Any");
        }
        else
        {
            SetPanelRenderColor(15);
            PANEL_RENDER_STRING(items[TP_FILTER_WIDTH].x, items[TP_FILTER_WIDTH].y, "%d", filter.width);
        }
    }

    if ( ShouldRenderInactiveTextField(&texturePanel, TP_FILTER_HEIGHT) )
    {
        if ( filter.height <= 0 )
        {
            SetPanelRenderColor(8);
            PANEL_RENDER_STRING(items[TP_FILTER_HEIGHT].x, items[TP_FILTER_HEIGHT].y, "Any");
        }
        else
        {
            SetPanelRenderColor(15);
            PANEL_RENDER_STRING(items[TP_FILTER_HEIGHT].x, items[TP_FILTER_HEIGHT].y, "%d", filter.height);
        }
    }

    if ( ShouldRenderInactiveTextField(&texturePanel, TP_FILTER_NAME) )
    {
        SetPanelRenderColor(15);
        PANEL_RENDER_STRING(items[TP_FILTER_NAME].x, items[TP_FILTER_NAME].y, "%s", filter.name);
    }

    //
    // Selected texture info
    //

    SetPanelRenderColor(11);
    PANEL_RENDER_STRING(items[TP_NAME].x, items[TP_NAME].y,
                        "%s",
                        currentTexture);
    PANEL_RENDER_STRING(items[TP_SIZE].x, items[TP_SIZE].y,
                        "%d x %d",
                        currentWidth, currentHeight);


    // Scroll bar control

    int x = SCROLL_BAR_COL * FONT_WIDTH;
    float percent = (float)paletteTopY / maxPaletteTopY;
    int barHeight = (SCROLL_BAR_BOTTOM - SCROLL_BAR_TOP) * FONT_HEIGHT;
    int y = (SCROLL_BAR_TOP * FONT_HEIGHT) + barHeight * percent;
    SetPanelRenderColor(15);
    RenderChar(x, y, 8);

    // The border around the texture palette (in window space)

    SDL_RenderSetViewport(renderer, &texturePanel.location);

    SDL_RenderSetViewport(renderer, NULL);
    SetPanelRenderColor(8);
    SDL_RenderDrawRect(renderer, &paletteRect);
}

void LoadTexturePanel(void)
{
    LoadPanelConsole(&texturePanel, PANEL_DATA_DIRECTORY"texture.panel");
    texturePanel.render = RenderTexturePanel;
    texturePanel.processEvent = ProcessTexturePanelEvent;
    texturePanel.textEditingCompletionHandler = FinishTextEditing;
    texturePanel.items = items;
    texturePanel.numItems = TP_COUNT;

    paletteRectRelative = (SDL_Rect)
    {
        .x = 2 * FONT_WIDTH,
        .y = 2 * FONT_HEIGHT,
        .w = 38 * FONT_WIDTH,
        .h = 37 * FONT_HEIGHT
    };

    UpdatePaletteTextureLocations();
}
