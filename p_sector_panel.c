//
//  sector_panel.c
//  de
//
//  Created by Thomas Foster on 6/24/23.
//

#include "p_sector_panel.h"
#include "p_sector_specials_panel.h"
#include "p_flats_panel.h"
#include "p_panel.h"
#include "p_stack.h"

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

SectorDef baseSectordef = {
    .floorHeight = 0,
    .ceilingHeight = 200,
    .lightLevel = 255,
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
    OpenPanel(&sectorPanel);

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

void DoSectorPanelAction(void)
{
    SectorDef * def = &baseSectordef;

    switch ( sectorPanel.selection )
    {
        case SP_FLOOR_FLAT:
            OpenFlatsPanel(SELECTING_FLOOR);
            break;

        case SP_CEILING_FLAT:
            OpenFlatsPanel(SELECTING_CEILING);
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
            OpenPanel(&sectorSpecialsPanel);
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
}

bool ProcessSectorPanelEvent(const SDL_Event * event)
{
    SectorDef * def = &baseSectordef;

    if ( DidClickOnItem(event, &sectorPanel) )
    {
        DoSectorPanelAction();
        return true;
    }

    switch ( event->type )
    {
        case SDL_MOUSEMOTION:
            if ( lightMeter.isDragging ) {
                int position = GetScrollbarHandlePosition(&lightMeter,
                                                sectorPanel.textLocation.x);
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
                    ScrollToPosition(&lightMeter, sectorPanel.textLocation.x);
                    def->lightLevel = position * LIGHT_METER_TICK;
                    return true;
                }
            }
            return false;
        }

        case SDL_MOUSEBUTTONUP:
            if ( event->button.button == SDL_BUTTON_LEFT
                && lightMeter.isDragging )
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
    SetPanelRenderColor(11);
    PANEL_RENDER_STRING(items[SP_FLOOR_FLAT].x,
                        items[SP_FLOOR_FLAT].y,
                        "%s", sector->floorFlat);
    PANEL_RENDER_STRING(items[SP_CEILING_FLAT].x,
                        items[SP_CEILING_FLAT].y,
                        "%s", sector->ceilingFlat);

    // Special
    SetPanelRenderColor(11);
    PANEL_RENDER_STRING(items[SP_SPECIAL].x,
                        items[SP_SPECIAL].y,
                        "%s", GetSpecialName(sector->special));
}

#pragma mark -

void LoadSectorPanel(void)
{
    LoadPanelConsole(&sectorPanel, PANEL_DATA_DIRECTORY "sector.panel");

//    sectorPanel.location.x = PANEL_SCREEN_MARGIN;
//    sectorPanel.location.y = PANEL_SCREEN_MARGIN;
    sectorPanel.processEvent = ProcessSectorPanelEvent;
    sectorPanel.render = RenderSectorPanel;
    sectorPanel.items = items;
    sectorPanel.numItems = SP_NUM_ITEMS;
    sectorPanel.selection = 1;
    sectorPanel.textEditingCompletionHandler = TextInputCompletionHandler;
}
