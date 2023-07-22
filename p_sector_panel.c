//
//  sector_panel.c
//  de
//
//  Created by Thomas Foster on 6/24/23.
//

#include "p_sector_panel.h"
#include "p_sector_special_panel.h"
#include "p_panel.h"

#include "array.h"
#include "m_map.h"
#include "flat.h"
#include "doomdata.h"

#define MAX_SELECTED_SECTORDEFS 1024
#define LIGHT_METER_ROW 14
#define LIGHT_METER_FIRST_COL 2
#define LIGHT_METER_LAST_COL 33
#define LIGHT_METER_TICK 8 // Each meter tick is 8 light levels.

Panel sectorPanel;
Panel flatsPanel;

SectorDef baseSectordef = {
    .floorHeight = 0,
    .ceilingHeight = 200,
    .lightLevel = 255,
};

static enum { SELECTING_FLOOR, SELECTING_CEILING } flatSelection;

static SDL_Rect paletteRectRelative;

static Scrollbar scrollBar = {
    .type = SCROLLBAR_VERTICAL,
    .location = 34,
    .min = 2,
    .max = 36
};

static int headroom;


static Scrollbar lightMeter = {
    .type = SCROLLBAR_HORIZONTAL,
    .location = 20,
    .min = 2,
    .max = 33,
};

enum {
    SP_FLOOR_HEIGHT = 1,
    SP_CEILING_HEIGHT,
    SP_HEADROOM,
    SP_SPECIAL,
    SP_TAG,
    SP_SUGGEST,
    SP_LIGHT,
    SP_FLOOR_FLAT,
    SP_CEILING_FLAT,

    SP_NUM_ITEMS
};

static PanelItem items[SP_NUM_ITEMS] =
{
    [0] = { 0 },
    [SP_FLOOR_HEIGHT]   = { 10, 12, 5 },
    [SP_CEILING_HEIGHT] = { 10, 4, 5 },
    [SP_HEADROOM]       = { 10, 6, 5 },
    [SP_SPECIAL]        = { 2, 24, 32 },
    [SP_TAG]            = { 7, 25, 4 },
    [SP_SUGGEST]        = { 13, 25, 7 },
    [SP_LIGHT]          = { 2, 19, 4 },
    [SP_FLOOR_FLAT]     = { 8, 16, 8, true, 22, 11, 33, 16 },
    [SP_CEILING_FLAT]   = { 8, 8, 8, true, 22, 3, 33, 8 }
};

static void ScrollToSelected(void);

void SectorPanelApplyChange(void)
{
    Line * line;
    FOR_EACH(line, map.lines)
    {
        if ( line->deleted )
            continue;
        else if ( line->selected )
            line->sides[line->selected - 1].sectorDef = baseSectordef;
    }
}

SectorDef GetBaseSectordef(void)
{
    if ( baseSectordef.floorFlat[0] == '\0' )
        GetFlatName(0, baseSectordef.floorFlat);

    if ( baseSectordef.ceilingFlat[0] == '\0' )
        GetFlatName(0, baseSectordef.ceilingFlat);

    return baseSectordef;
}

void OpenSectorPanel(void)
{
    OpenPanel(&sectorPanel, NULL);

    Line * line;
    FOR_EACH(line, map.lines)
    {
        Sidedef * side = SelectedSide(line);
        if ( side )
        {
            baseSectordef = side->sectorDef;
            break;
        }
    }
}

static void UpdateFlatLocations(void)
{
    Flat * flat = flats->data;

    int x = PALETTE_ITEM_MARGIN;
    int y = PALETTE_ITEM_MARGIN;
    int maxY = 0;

    for ( int i = 0; i < flats->count; i++, flat++ )
    {
        // TODO: if filtered out, continue

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

#pragma mark -

static void TextInputCompletionHandler(void)
{
    SectorDef * def = &baseSectordef;

    switch ( sectorPanel.selection )
    {
        case SP_FLOOR_HEIGHT:
            SectorPanelApplyChange();
            break;
        case SP_CEILING_HEIGHT:
            SectorPanelApplyChange();
            break;
        case SP_HEADROOM:
            if ( headroom >= 0 )
            {
                def->ceilingHeight = def->floorHeight + headroom;
                SectorPanelApplyChange();
            }
            break;
        case SP_LIGHT:
            def->lightLevel = SDL_clamp(def->lightLevel, 0, 255);
            SectorPanelApplyChange();
            break;
        default:
            break;
    }
}

bool ProcessSectorPanelEvent(const SDL_Event * event)
{
    SectorDef * def = &baseSectordef;

    if ( IsActionEvent(event, &sectorPanel) )
    {
        switch ( sectorPanel.selection )
        {
            case SP_FLOOR_FLAT:
                flatSelection = SELECTING_FLOOR;
                OpenPanel(&flatsPanel, NULL);
                ScrollToSelected();
                break;

            case SP_CEILING_FLAT:
                flatSelection = SELECTING_CEILING;
                OpenPanel(&flatsPanel, NULL);
                ScrollToSelected();
                break;

            case SP_FLOOR_HEIGHT:
                StartTextEditing(&sectorPanel,
                                 SP_FLOOR_HEIGHT,
                                 &def->floorHeight,
                                 VALUE_INT);
                break;

            case SP_CEILING_HEIGHT:
                StartTextEditing(&sectorPanel,
                                 SP_CEILING_HEIGHT,
                                 &def->ceilingHeight,
                                 VALUE_INT);
                break;

            case SP_HEADROOM:
                StartTextEditing(&sectorPanel, SP_HEADROOM, &headroom, VALUE_INT);
                break;

            case SP_TAG:
                StartTextEditing(&sectorPanel, SP_TAG, &def->tag, VALUE_INT);
                break;

            case SP_LIGHT:
                StartTextEditing(&sectorPanel,
                                 SP_LIGHT,
                                 &def->lightLevel,
                                 VALUE_INT);
                break;

            case SP_SPECIAL:
                OpenPanel(&sectorSpecialsPanel, NULL);
                break;

            case SP_SUGGEST:
            {
                Line * line = map.lines->data;
                int maxTag = -1;
                for ( int i = 0; i < map.lines->count; i++, line++ )
                {
                    int numSides = line->flags & ML_TWOSIDED ? 2 : 1;
                    for ( int s = 0; s < numSides; s++ )
                    {
                        if ( line->sides[s].sectorDef.tag > maxTag )
                            maxTag = line->sides[s].sectorDef.tag;
                    }
                }
                def->tag = maxTag + 1;
                SectorPanelApplyChange();
                break;
            }

            default:
                break;
        }
        return true;
    }

    switch ( event->type )
    {
        case SDL_MOUSEMOTION:
            if ( lightMeter.isDragging ) {
                int position = GetPositionInScrollbar(&lightMeter,
                                                      sectorPanel.textLocation.x,
                                                      sectorPanel.textLocation.y);
                def->lightLevel = position * LIGHT_METER_TICK;
                def->lightLevel = SDL_clamp(def->lightLevel, 0, 255);
                return true;
            }
            return false;

        case SDL_MOUSEBUTTONDOWN:
        {
            if ( event->button.button == SDL_BUTTON_LEFT )
            {
                int position = GetPositionInScrollbar(&lightMeter,
                                                      sectorPanel.textLocation.x,
                                                      sectorPanel.textLocation.y);

                if ( position != -1 )
                {
                    lightMeter.isDragging = true;
                    def->lightLevel = position * LIGHT_METER_TICK;
                    return true;
                }
            }
            return false;
        }

        case SDL_MOUSEBUTTONUP:
            if ( event->button.button == SDL_BUTTON_LEFT && lightMeter.isDragging )
            {
                lightMeter.isDragging = false;
                SectorPanelApplyChange();
                return true;
            }
            return false;

        default:
            return false;
    }

    return false;
}

void RenderSectorPanel(void)
{
    SectorDef * sector = &baseSectordef;

    RenderFlat(sector->floorFlat, 22 * FONT_WIDTH, 11 * FONT_HEIGHT, 1.5f);
    RenderFlat(sector->ceilingFlat, 22 * FONT_WIDTH, 3 * FONT_HEIGHT, 1.5f);

    SetPanelRenderColor(15);

    // Height

    if ( ShouldRenderInactiveTextField(&sectorPanel, SP_FLOOR_HEIGHT) )
    {
        PANEL_RENDER_STRING(items[SP_FLOOR_HEIGHT].x,
                            items[SP_FLOOR_HEIGHT].y,
                            "%d", sector->floorHeight);
    }

    if ( ShouldRenderInactiveTextField(&sectorPanel, SP_CEILING_HEIGHT) )
    {
        PANEL_RENDER_STRING(items[SP_CEILING_HEIGHT].x,
                            items[SP_CEILING_HEIGHT].y,
                            "%d", sector->ceilingHeight);
    }

    if ( ShouldRenderInactiveTextField(&sectorPanel, SP_HEADROOM) )
    {
        PANEL_RENDER_STRING(items[SP_HEADROOM].x,
                            items[SP_HEADROOM].y,
                            "%d", sector->ceilingHeight - sector->floorHeight);
    }
    // Tag

    if ( ShouldRenderInactiveTextField(&sectorPanel, SP_TAG) )
    {
        PANEL_RENDER_STRING(items[SP_TAG].x,
                            items[SP_TAG].y,
                            "%d", sector->tag);
    }

    // Light level

    if ( ShouldRenderInactiveTextField(&sectorPanel, SP_LIGHT) )
    {
        PANEL_RENDER_STRING(items[SP_LIGHT].x,
                            items[SP_LIGHT].y,
                            "%d", sector->lightLevel);
    }

    int x = lightMeter.min + sector->lightLevel / 8;
    RenderChar(x * FONT_WIDTH, lightMeter.location * FONT_HEIGHT, 8);

    // Flat names

    PANEL_RENDER_STRING(items[SP_FLOOR_FLAT].x,
                        items[SP_FLOOR_FLAT].y,
                        "%s", sector->floorFlat);
    PANEL_RENDER_STRING(items[SP_CEILING_FLAT].x,
                        items[SP_CEILING_FLAT].y,
                        "%s", sector->ceilingFlat);

    // Special
    PANEL_RENDER_STRING(items[SP_SPECIAL].x,
                        items[SP_SPECIAL].y,
                        "%s", GetSpecialName(sector->special));
}

#pragma mark - FLATS PANEL

static void ScrollToSelected(void)
{
    Flat * flat;
    FOR_EACH(flat, flats)
    {
        if ( (flatSelection == SELECTING_FLOOR
            && strcmp(flat->name, baseSectordef.floorFlat) == 0 )
            ||
            (flatSelection == SELECTING_CEILING
                && strcmp(flat->name, baseSectordef.ceilingFlat) == 0) )
        {
            scrollBar.scrollPosition = flat->rect.y - PALETTE_ITEM_MARGIN;
        }
    }
}

static bool ProcessFlatsPanelEvent(const SDL_Event * event)
{
    switch ( event->type )
    {
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

                else if ( SDL_PointInRect(&flatsPanel.mouseLocation,
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

                        if ( SDL_PointInRect(&loc, &flat->rect) )
                        {
                            if ( flatSelection == SELECTING_FLOOR )
                            {
                                GetFlatName(i, baseSectordef.floorFlat);
                                SectorPanelApplyChange();
                            }
                            else if ( flatSelection == SELECTING_CEILING )
                            {
                                GetFlatName(i, baseSectordef.ceilingFlat);
                                SectorPanelApplyChange();
                            }

                            if ( event->button.clicks == 2 )
                                topPanel--;
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
        if (   flat->rect.y + flat->rect.h >= top
            && flat->rect.y < bottom )
        {
            SDL_Rect dest = flat->rect;
            dest.y -= scrollBar.scrollPosition;
            SDL_RenderCopy(renderer, flat->texture, NULL, &dest);

            if ( (flatSelection == SELECTING_FLOOR
                && strncmp(baseSectordef.floorFlat, flat->name, 8) == 0)
                ||
                (flatSelection == SELECTING_CEILING
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
}

#pragma mark -

void LoadSectorPanel(void)
{
    sectorPanel = LoadPanel(PANEL_DATA_DIRECTORY "sector.panel");
    sectorPanel.location.x = PANEL_SCREEN_MARGIN;
    sectorPanel.location.y = PANEL_SCREEN_MARGIN;
    sectorPanel.processEvent = ProcessSectorPanelEvent;
    sectorPanel.render = RenderSectorPanel;
    sectorPanel.items = items;
    sectorPanel.numItems = SP_NUM_ITEMS;
    sectorPanel.selection = 1;
    sectorPanel.textEditingCompletionHandler = TextInputCompletionHandler;

    flatsPanel = LoadPanel(PANEL_DATA_DIRECTORY "flat_palette.panel");
    flatsPanel.render = RenderFlatsPanel;
    flatsPanel.processEvent = ProcessFlatsPanelEvent;

    // The palette rect relative to the flat panel.
    paletteRectRelative.x = 2 * FONT_WIDTH;
    paletteRectRelative.y = 1 * FONT_HEIGHT;
    paletteRectRelative.w = 32 * FONT_WIDTH;
    paletteRectRelative.h = FONT_HEIGHT * 37;

    UpdateFlatLocations();
}
