//
//  p_flats_panel.c
//  de
//
//  Created by Thomas Foster on 7/24/23.
//

#include "p_flats_panel.h"

#include "p_sector_panel.h"
#include "p_panel.h"
#include "p_stack.h"
#include "m_line.h"
#include "flat.h"

extern SectorDef baseSectordef;

static Panel flatsPanel;
static FlatSelection selection;
static SDL_Rect paletteRectRelative;
static char filter[9];

static Scrollbar scrollBar = {
    .type = SCROLLBAR_VERTICAL,
    .location = 34,
    .min = 2,
    .max = 36
};

enum
{
    FP_FILTER,
    FP_NAME,
    FP_NUM_ITEMS
};

static PanelItem items[FP_NUM_ITEMS] =
{
    [FP_FILTER] = { 8, 39, 9, true, 2, 39, 16, 39 },
    [FP_NAME] = { 27, 39, 8 },
};



static void ScrollToSelected(void)
{
    Flat * flat;
    FOR_EACH(flat, flats)
    {
        if ( (selection == SELECTING_FLOOR
            && strcmp(flat->name, baseSectordef.floorFlat) == 0 )
            ||
            (selection == SELECTING_CEILING
                && strcmp(flat->name, baseSectordef.ceilingFlat) == 0) )
        {
            scrollBar.scrollPosition = flat->rect.y - PALETTE_ITEM_MARGIN;
        }
    }
}

static const char * FlatName(void)
{
    if ( selection == SELECTING_FLOOR )
        return baseSectordef.floorFlat;
    else
        return baseSectordef.ceilingFlat;
}

bool IsFilteredOut(const char * name)
{
    return filter[0] != '\0' && strcasestr(name, filter) == NULL;
}

static void UpdateFlatLocations(void)
{
    Flat * flat = flats->data;

    int x = PALETTE_ITEM_MARGIN;
    int y = PALETTE_ITEM_MARGIN;
    int maxY = 0;

    for ( int i = 0; i < flats->count; i++, flat++ )
    {
        if ( IsFilteredOut(flat->name) )
            continue;

        if ( x + 64 > paletteRectRelative.w - PALETTE_ITEM_MARGIN )
        {
            // Start a new row.
            x = PALETTE_ITEM_MARGIN;
            y += 64 + PALETTE_ITEM_MARGIN;
        }

        flat->rect.x = x;
        flat->rect.y = y;

        x += 64 + PALETTE_ITEM_MARGIN;

        maxY = flat->rect.y + 64;
    }

    scrollBar.maxScrollPosition = maxY + PALETTE_ITEM_MARGIN - paletteRectRelative.h;
}

void OpenFlatsPanel(FlatSelection sel)
{
    selection = sel;
    OpenPanel(&flatsPanel);
    ScrollToSelected();
    filter[0] = '\0';

    UpdateFlatLocations();
}

static void StartFilterTextEditing(void)
{
    StartTextEditing(&flatsPanel,
                     flatsPanel.selection,
                     filter,
                     VALUE_STRING);
}

static void DoFlatsPanelAction(void)
{
    switch ( flatsPanel.selection )
    {
        case FP_FILTER:
            StartFilterTextEditing();
            break;
        case FP_NAME:
            ScrollToSelected();
            break;

        default:
            break;
    }
}

static bool ProcessFlatsPanelEvent(const SDL_Event * event)
{
    if ( DidClickOnItem(event, &flatsPanel) )
    {
        DoFlatsPanelAction();
        return true;
    }

    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_n:
                    StartFilterTextEditing();
                    return true;
                case SDLK_BACKSPACE:
                    filter[0] = '\0';
                    UpdateFlatLocations();
                    return true;
                default:
                    return false;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
        {
            if ( event->button.button == SDL_BUTTON_LEFT )
            {
                if ( GetPositionInScrollbar(&scrollBar,
                                            flatsPanel.textLocation.x,
                                            flatsPanel.textLocation.y) != -1 )
                {
                    scrollBar.isDragging = true;
                    ScrollToPosition(&scrollBar, flatsPanel.textLocation.y);
                    return true;
                }

                // Clicked on a flat in the palette?
                if ( SDL_PointInRect(&flatsPanel.mouseLocation,
                                          &paletteRectRelative) )
                {
                    SDL_Point loc = flatsPanel.mouseLocation;

                    // Convert to palette space
                    loc.x -= paletteRectRelative.x;
                    loc.y -= paletteRectRelative.y;
                    loc.y += scrollBar.scrollPosition;

                    Flat * flat = flats->data;
                    for ( int i = 0; i < flats->count; i++, flat++ )
                    {
                        // TODO: if is filtered out, continue
                        if ( IsFilteredOut(flat->name) )
                            continue;

                        if ( SDL_PointInRect(&loc, &flat->rect) )
                        {
                            if ( selection == SELECTING_FLOOR )
                            {
                                GetFlatName(i, baseSectordef.floorFlat);
                                SectorPanelApplyChange();
                            }
                            else if ( selection == SELECTING_CEILING )
                            {
                                GetFlatName(i, baseSectordef.ceilingFlat);
                                SectorPanelApplyChange();
                            }

                            if ( event->button.clicks == 2 )
                                CloseTopPanel();
                        }
                    }
                }

                return true;
            }

            return false;
        }

        case SDL_MOUSEMOTION:
            if ( scrollBar.isDragging )
            {
                ScrollToPosition(&scrollBar, flatsPanel.textLocation.y);
                return true;
            }
            return false;

        case SDL_MOUSEBUTTONUP:
            if ( event->button.button == SDL_BUTTON_LEFT
                && scrollBar.isDragging )
            {
                scrollBar.isDragging = false;
                return true;
            }
            return false;

        case SDL_MOUSEWHEEL:
            scrollBar.scrollPosition += event->wheel.y * 16;
            CLAMP(scrollBar.scrollPosition, 0, scrollBar.maxScrollPosition);
            return true;

        default:
            return false;
    }
}

void RenderFlatsPanel(void)
{
    SDL_Rect flatPanelLocation = paletteRectRelative;
    flatPanelLocation.x += flatsPanel.location.x;
    flatPanelLocation.y += flatsPanel.location.y;

    // Palette Outline

    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderDrawRect(renderer, &paletteRectRelative);

    // Palette

    SDL_RenderSetViewport(renderer, &flatPanelLocation);

    int top = scrollBar.scrollPosition;
    int bottom = top + paletteRectRelative.h;

    // Render visible flats.
    Flat * flat = flats->data;
    for ( int i = 0; i < flats->count; i++, flat++ )
    {
        if ( IsFilteredOut(flat->name) )
            continue;

        if (   flat->rect.y + flat->rect.h >= top
            && flat->rect.y < bottom )
        {
            SDL_Rect dest = flat->rect;
            dest.y -= scrollBar.scrollPosition;
            SDL_RenderCopy(renderer, flat->texture, NULL, &dest);

            if ( (selection == SELECTING_FLOOR
                  && strncmp(baseSectordef.floorFlat, flat->name, 8) == 0)
                ||
                (selection == SELECTING_CEILING
                 && strncmp(baseSectordef.ceilingFlat, flat->name, 8) == 0) )
            {
                // TODO: factor out (texture palette is the same)
                SDL_Rect box = flat->rect;
                box.y -= scrollBar.scrollPosition;

                int margin = SELECTION_BOX_MARGIN;
                box.x -= margin;
                box.y -= margin;
                box.w += margin * 2;
                box.h += margin * 2;

                // TODO: this should be a default
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                DrawRect(&(SDL_FRect){ box.x, box.y, box.w, box.h },
                         SELECTION_BOX_THICKNESS);
            }
        }
    }

    SDL_RenderSetViewport(renderer, &flatsPanel.location);

    // Scrollbar

    int x = scrollBar.location * FONT_WIDTH;
    float percent = (float)scrollBar.scrollPosition / scrollBar.maxScrollPosition;
    int barHeight = (scrollBar.max - scrollBar.min) * FONT_HEIGHT;
    int y = scrollBar.min * FONT_HEIGHT + barHeight * percent;
    SetPanelRenderColor(15);
    RenderChar(x, y, 8);

    // Name

    const char * name;
    if ( selection == SELECTING_FLOOR )
        name = baseSectordef.floorFlat;
    else
        name = baseSectordef.ceilingFlat;

    x = items[FP_NAME].x;
    y = items[FP_NAME].y;
    SetPanelRenderColor(11);
    PANEL_RENDER_STRING(x, y, "%s", name);

    x = items[FP_FILTER].x;
    y = items[FP_FILTER].y;

    if ( ShouldRenderInactiveTextField(&flatsPanel, FP_FILTER) )
    {
        if ( filter[0] )
        {
            SetPanelRenderColor(15);
            PANEL_RENDER_STRING(x, y, "%s", filter);
        }
        else
        {
            SetPanelRenderColor(8);
            PANEL_RENDER_STRING(x, y, "Any");
        }
    }
}

static void TextEditingCompletionHandler(void)
{
    UpdateFlatLocations();
    scrollBar.scrollPosition = 0;
}

void LoadFlatsPanel(void)
{
    LoadPanelConsole(&flatsPanel, PANEL_DATA_DIRECTORY "flat_palette.panel");
    flatsPanel.render = RenderFlatsPanel;
    flatsPanel.processEvent = ProcessFlatsPanelEvent;
    flatsPanel.items = items;
    flatsPanel.numItems = FP_NUM_ITEMS;
    flatsPanel.textEditingCompletionHandler = TextEditingCompletionHandler;

    // The palette rect relative to the flat panel.
    paletteRectRelative.x = 2 * FONT_WIDTH;
    paletteRectRelative.y = 1 * FONT_HEIGHT;
    paletteRectRelative.w = 32 * FONT_WIDTH;
    paletteRectRelative.h = FONT_HEIGHT * 37;

    UpdateFlatLocations();
}
