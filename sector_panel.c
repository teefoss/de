//
//  sector_panel.c
//  de
//
//  Created by Thomas Foster on 6/24/23.
//

#include "sector_panel.h"
#include "panel.h"
#include "array.h"
#include "map.h"
#include "flat.h"
#include "doomdata.h"

#define MAX_SELECTED_SECTORDEFS 1024
#define LIGHT_METER_ROW 14
#define LIGHT_METER_FIRST_COL 2
#define LIGHT_METER_LAST_COL 33
#define LIGHT_METER_TICK 8 // Each meter tick is 8 light.

Panel sectorPanel;
Panel sectorSpecialsPanel;
Panel flatsPanel;

static SDL_Rect paletteRect;

static Scrollbar scrollBar = {
    .type = SCROLLBAR_VERTICAL,
    .location = 34,
    .min = 2,
    .max = 36
};

static int numSelectedSectorDefs;
static SectorDef * selectedSectorDefs[MAX_SELECTED_SECTORDEFS];
static int headroom;

// Cursor text cell.
static int cx = -1;
static int cy = -1;

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

typedef struct
{
    int id;
    const char * name;
} SectorSpecial;

SectorSpecial sectorSpecials[] =
{
    { 0,    "None" },
    { 7,    "Damage 5%" },
    { 5,    "Damage 10%" },
    { 16,   "Damage 20%" },
    { 4,    "Damage 20% and Strobe" },
    { 11,   "Damage 20% and Exit" },

    { 3,    "Light Strobe Slow" },
    { 12,   "Light Strobe Slow Sync" },
    { 2,    "Light Strobe Fast" },
    { 13,   "Light Strobe Fast Sync" },
    { 1,    "Light Blink Random" },
    { 17,   "Light Fire Flickering" }, // Not in Doom 1.
    { 8,    "Light Glowing" },

    { 10,   "Door Close in 30 Sec." },
    { 14,   "Door Open in 5 Min." },

    { 6,    "Ceiling Crush and Raise" },
    { 9,    "Secret Area" },
};

static PanelItem specialItems[] =
{
    { .y = 1 },
    { .y = 3 },
    { .y = 4 },
    { .y = 5 },
    { .y = 6 },
    { .y = 7 },
    { .y = 9 },
    { .y = 10 },
    { .y = 11 },
    { .y = 12 },
    { .y = 13 },
    { .y = 14 },
    { .y = 16 },
    { .y = 17 },
    { .y = 19 },
    { .y = 20 },
};

static PanelItem specialItems2[] =
{
    { .y = 1 },
    { .y = 3 },
    { .y = 4 },
    { .y = 5 },
    { .y = 6 },
    { .y = 7 },
    { .y = 9 },
    { .y = 10 },
    { .y = 11 },
    { .y = 12 },
    { .y = 13 },
    { .y = 14 },
    { .y = 15 },
    { .y = 17 },
    { .y = 18 },
    { .y = 20 },
    { .y = 21 },
};

static PanelItem items[SP_NUM_ITEMS] =
{
    [0] = { 0 },
    [SP_FLOOR_HEIGHT]   = {
        10, 12, 5,
        SP_CEILING_FLAT, SP_FLOOR_FLAT, -1, SP_FLOOR_FLAT,
    },
    [SP_CEILING_HEIGHT] = {
        10, 4, 5,
        -1, SP_HEADROOM, -1, SP_CEILING_FLAT,
    },
    [SP_HEADROOM] = {
        10, 6, 5,
        SP_CEILING_HEIGHT, SP_CEILING_FLAT, -1, SP_CEILING_FLAT,
    },
    [SP_SPECIAL] = {
        2, 24, 32,
        SP_LIGHT, SP_TAG, -1, SP_SUGGEST,
    },
    [SP_TAG] = {
        7, 25, 4,
        SP_SPECIAL, -1, -1, SP_SUGGEST,
    },
    [SP_SUGGEST] = {
        13, 25, 7,
        SP_SPECIAL, -1, SP_TAG, -1,
    },
    [SP_LIGHT] = {
        2, 19, 4,
        SP_FLOOR_FLAT, SP_SPECIAL, -1, -1,
    },
    [SP_FLOOR_FLAT] = {
        8, 16, 8,
        SP_FLOOR_HEIGHT, SP_LIGHT, -1, -1,
    },
    [SP_CEILING_FLAT] = {
        8, 8, 8,
        -1, SP_HEADROOM, SP_FLOOR_FLAT, -1,
    }
};

void OpenSectorPanel(void)
{
    OpenPanel(&sectorPanel, NULL);
    numSelectedSectorDefs = 0;

    Line * lines = map.lines->data;
    for ( Line * line = lines; line < lines + map.lines->count; line++ )
    {
        Side * side = SelectedSide(line);
        if ( side )
            selectedSectorDefs[numSelectedSectorDefs++] = &side->sectorDef;
    }
}

static void UpdateSelectedSectorDefs(void)
{
    for ( int i = 1; i < numSelectedSectorDefs; i++ )
        *selectedSectorDefs[i] = *selectedSectorDefs[0];
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

        if ( x + 64 > paletteRect.w - PALETTE_ITEM_MARGIN )
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

    scrollBar.maxScrollPosition = maxY + PALETTE_ITEM_MARGIN - paletteRect.h;
}

#pragma mark -

static void TextInputCompletionHandler(void)
{
    SectorDef * def = selectedSectorDefs[0];

    switch ( sectorPanel.selection )
    {
        case SP_FLOOR_HEIGHT:
        case SP_CEILING_HEIGHT:
            UpdateSelectedSectorDefs();
            break;
        case SP_HEADROOM:
            if ( headroom >= 0 )
            {
                def->ceilingHeight = def->floorHeight + headroom;
                UpdateSelectedSectorDefs();
            }
            break;
        case SP_LIGHT:
            def->lightLevel = SDL_clamp(def->lightLevel, 0, 255);
            UpdateSelectedSectorDefs();
            break;
        default:
            break;
    }
}

bool ProcessSectorPanelEvent(const SDL_Event * event)
{
    SectorDef * def = selectedSectorDefs[0];

    if ( IsActionEvent(event, &sectorPanel) )
    {
        switch ( sectorPanel.selection )
        {
            case SP_FLOOR_FLAT:
                OpenPanel(&flatsPanel, NULL);
                break;

            case SP_CEILING_FLAT:
                OpenPanel(&flatsPanel, NULL);
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
                UpdateSelectedSectorDefs();
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
            cx = (event->motion.x - sectorPanel.location.x) / FONT_WIDTH;
            cy = (event->motion.y - sectorPanel.location.y) / FONT_HEIGHT;
            if ( lightMeter.isDragging ) {
                int position = GetPositionInScrollbar(&lightMeter, cx, cy);
                def->lightLevel = position * LIGHT_METER_TICK;
                def->lightLevel = SDL_clamp(def->lightLevel, 0, 255);
                return true;
            }
            return false;

        case SDL_MOUSEBUTTONDOWN:
        {
            if ( event->button.button == SDL_BUTTON_LEFT )
            {
                int position = GetPositionInScrollbar(&lightMeter, cx, cy);

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
                UpdateSelectedSectorDefs();
                return true;
            }
            return false;

        default:
            return false;
    }

    return false;
}

static const char * GetSpecialName(int id)
{
    for ( int i = 0; i < sectorSpecialsPanel.numItems; i++ )
    {
        if ( sectorSpecials[i].id == id )
            return sectorSpecials[i].name;
    }

    return "";
}

static bool ProcessSpecialPanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event, &sectorSpecialsPanel) )
    {
        int id = sectorSpecials[sectorSpecialsPanel.selection].id;
        selectedSectorDefs[0]->special = id;
        UpdateSelectedSectorDefs();
        topPanel--;
        return true;
    }

    return false;
}

static bool ProcessFlatsPanelEvent(const SDL_Event * event)
{
//    if ( IsActionEvent(event, &flatsPanel) )
//    {
//        return true;
//    }

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

void RenderSectorPanel(void)
{
    SectorDef * sector = selectedSectorDefs[0];

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

void RenderFlatsPanel(void)
{
    SDL_RenderSetViewport(renderer, NULL);
//    SDL_RenderSetViewport(renderer, &flatsPanel.location);
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderDrawRect(renderer, &paletteRect);

    SDL_RenderSetViewport(renderer, &paletteRect);

    int top = scrollBar.scrollPosition;
    int bottom = top + paletteRect.h;

    Flat * flat = flats->data;
    for ( int i = 0; i < flats->count; i++, flat++ )
    {
        if (   flat->rect.y + flat->rect.h >= top // Visible?
            && flat->rect.y < bottom )
        {
            SDL_Rect dest = flat->rect;
            dest.y -= scrollBar.scrollPosition;
            SDL_RenderCopy(renderer, flat->texture, NULL, &dest);

            // TODO: selection box
        }
    }

    SDL_RenderSetViewport(renderer, &flatsPanel.location);

    int x = scrollBar.location * FONT_WIDTH;
    float percent = (float)scrollBar.scrollPosition / scrollBar.maxScrollPosition;
    int barHeight = (scrollBar.max - scrollBar.min) * FONT_HEIGHT;
    int y = scrollBar.min * FONT_HEIGHT + barHeight * percent;
    SetPanelRenderColor(15);
    RenderChar(x, y, 8);
}

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

    if ( editor.game == GAME_DOOM1 )
    {
        sectorSpecialsPanel = LoadPanel(PANEL_DATA_DIRECTORY "sector_specials.panel");
        sectorSpecialsPanel.numItems = SDL_arraysize(specialItems);
        sectorSpecialsPanel.items = specialItems;
    }
    else
    {
        sectorSpecialsPanel = LoadPanel(PANEL_DATA_DIRECTORY "sector_specials2.panel");
        sectorSpecialsPanel.numItems = SDL_arraysize(specialItems2);
        sectorSpecialsPanel.items = specialItems2;
    }

    for ( int i = 0; i < sectorSpecialsPanel.numItems; i++ )
    {
        sectorSpecialsPanel.items[i].x = 2;
        sectorSpecialsPanel.items[i].width = 23;
        sectorSpecialsPanel.items[i].up = i - 1;
        sectorSpecialsPanel.items[i].down = i + 1;
        sectorSpecialsPanel.items[i].left = -1;
        sectorSpecialsPanel.items[i].right = -1;
    }

    sectorSpecialsPanel.items[0].up = -1;
    sectorSpecialsPanel.items[sectorSpecialsPanel.numItems - 1].down = -1;
    sectorSpecialsPanel.location.x = sectorPanel.location.x + 2 * FONT_WIDTH;
    sectorSpecialsPanel.location.y = sectorPanel.location.y + 18 * FONT_HEIGHT;
    sectorSpecialsPanel.processEvent = ProcessSpecialPanelEvent;

    flatsPanel = LoadPanel(PANEL_DATA_DIRECTORY "flat_palette.panel");
    flatsPanel.location.x = sectorPanel.location.x + sectorPanel.location.w + PANEL_SCREEN_MARGIN;
    flatsPanel.location.y = PANEL_SCREEN_MARGIN;
    flatsPanel.render = RenderFlatsPanel;
    flatsPanel.processEvent = ProcessFlatsPanelEvent;

    paletteRect.x = flatsPanel.location.x + 2 * FONT_WIDTH;
    paletteRect.y = flatsPanel.location.y + 1 * FONT_HEIGHT;
    paletteRect.w = 32 * FONT_WIDTH;
    paletteRect.h = FONT_HEIGHT * 37;

    UpdateFlatLocations();
}
