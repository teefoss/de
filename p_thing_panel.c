//
//  thing_panel.c
//  de
//
//  Created by Thomas Foster on 6/19/23.
//

#include "p_thing_panel.h"
#include "doomdata.h"
#include "patch.h"
#include "m_map.h"

Panel thingPanel;
Panel thingCategoryPanel;
Panel thingPalette;

static int category; // Selected category index.
SDL_Rect thingPaletteRectOffsets; // The rect where things are displayed.

enum
{
    THP_TYPE = 1,
    THP_NW,
    THP_N,
    THP_NE,
    THP_W,
    THP_E,
    THP_SW,
    THP_S,
    THP_SE,
    THP_EASY,
    THP_NORMAL,
    THP_HARD,
    THP_AMBUSH,
    THP_NETWORK,
    THP_COUNT,
};

const char * directionNames[] =
{
    0, 0,
    "Northwest",
    "North",
    "Northeast",
    "West",
    "East",
    "Southwest",
    "South",
    "Southeast",
};

static PanelItem items[THP_COUNT] =
{
    [0] = { 0 },
    [THP_TYPE]      = { 2, 10, 25, true, 2, 3, 13, 8 },

    [THP_NW]        = { 2, 13, 2 },
    [THP_N]         = { 6, 13, 2 },
    [THP_NE]        = { 10, 13, 2 },
    [THP_W]         = { 2, 15, 2 },
    [THP_E]         = { 10, 15, 2 },
    [THP_SW]        = { 2, 17, 2 },
    [THP_S]         = { 6, 17, 2 },
    [THP_SE]        = { 10, 17, 2 },

    [THP_EASY]      = { 6, 20, 7  },
    [THP_NORMAL]    = { 6, 21, 7  },
    [THP_HARD]      = { 6, 22, 7  },
    [THP_AMBUSH]    = { 6, 23, 7  },
    [THP_NETWORK]   = { 6, 24, 7  },
};

static PanelItem categoryItems[THING_CATEGORY_COUNT];

SDL_Rect ThingPaletteRect(void)
{
    SDL_Rect rect = thingPaletteRectOffsets;
    rect.x += thingPalette.location.x;
    rect.y += thingPalette.location.y;

    return rect;
}

bool ProcessThingPanelEvent(const SDL_Event * event)
{
    Thing * thing = (Thing *)thingPanel.data;

    if ( IsActionEvent(event, &thingPanel) )
    {

        switch ( thingPanel.selection )
        {
            case THP_TYPE:
                OpenPanel(&thingCategoryPanel, NULL);
                break;

            case THP_E: thing->angle = 0; break;
            case THP_NE: thing->angle = 45; break;
            case THP_N: thing->angle = 90; break;
            case THP_NW: thing->angle = 135; break;
            case THP_W: thing->angle = 180; break;
            case THP_SW: thing->angle = 225; break;
            case THP_S: thing->angle = 270; break;
            case THP_SE: thing->angle = 315; break;

            case THP_EASY: thing->options ^= MTF_EASY; break;
            case THP_NORMAL: thing->options ^= MTF_NORMAL; break;
            case THP_HARD: thing->options ^= MTF_HARD; break;
            case THP_AMBUSH: thing->options ^= MTF_AMBUSH; break;
            case THP_NETWORK: thing->options ^= MTF_NETWORK; break;

            default:
                break;
        }
        return true;
    }

    return false;
}

bool ProcessThingCategoryPanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event, &thingCategoryPanel) )
    {
        category = thingCategoryPanel.selection;
        OpenPanel(&thingPalette, NULL);
        return true;
    }

    return false;
}

#pragma mark - RENDER

void RenderAngle(int itemIndex)
{
    PanelItem * item = &items[itemIndex];
    SetPanelRenderColor(11);
    RenderChar(item->x * FONT_WIDTH, item->y * FONT_HEIGHT, 219);
    RenderChar((item->x + 1) * FONT_WIDTH, item->y * FONT_HEIGHT, 219);

    SetPanelRenderColor(15);
    PANEL_RENDER_STRING(14, 13, "%s", directionNames[itemIndex]);
}

void RenderThingPanel(void)
{
    Thing * thing = (Thing *)thingPanel.data;
    ThingDef * def = GetThingDef(thing->type);

    SetPanelRenderColor(15);
    PANEL_RENDER_STRING(2, 1, "Thing %d", thing - (Thing *)map.things->data);

    // Thing Image

    SDL_Rect imageRect = {
        2 * FONT_WIDTH,
        3 * FONT_HEIGHT,
        12 * FONT_WIDTH,
        6 * FONT_HEIGHT
    };

    RenderPatchInRect(&def->patch, &imageRect);

    // Angle

    switch ( thing->angle )
    {
        case 0:   RenderAngle(THP_E); break;
        case 45:  RenderAngle(THP_NE); break;
        case 90:  RenderAngle(THP_N); break;
        case 135: RenderAngle(THP_NW); break;
        case 180: RenderAngle(THP_W); break;
        case 225: RenderAngle(THP_SW); break;
        case 270: RenderAngle(THP_S); break;
        case 315: RenderAngle(THP_SE); break;
        default:
            break;
    }

    SetPanelRenderColor(15);
    PANEL_RENDER_STRING(items[THP_TYPE].x, items[THP_TYPE].y, "%s", def->name);

    // Options

    RenderMark(&items[THP_EASY], thing->options & MTF_EASY);
    RenderMark(&items[THP_NORMAL], thing->options & MTF_NORMAL);
    RenderMark(&items[THP_HARD], thing->options & MTF_HARD);
    RenderMark(&items[THP_AMBUSH], thing->options & MTF_AMBUSH);
    RenderMark(&items[THP_NETWORK], thing->options & MTF_NETWORK);
}

void RenderThingPalette(void)
{
    SetPanelRenderColor(8);
    SDL_RenderSetViewport(renderer, NULL);

    SDL_Rect thingPaletteRect = ThingPaletteRect();
    SDL_RenderDrawRect(renderer, &thingPaletteRect); // DEBUG
    SDL_RenderSetViewport(renderer, &thingPaletteRect);

    Thing * thing = (Thing *)thingPanel.data;
    ThingCategoryInfo * info = &categoryInfo[category];
    int hoverThingDefIndex = -1;

    SDL_Point mouse = thingPalette.mouseLocation;
    SDL_Point mouseConverted; // To paletteRect
    mouseConverted.x = mouse.x - (thingPaletteRect.x - thingPalette.location.x);
    mouseConverted.y = mouse.y - (thingPaletteRect.y - thingPalette.location.y);

    for ( int i = 0; i < info->count; i++ )
    {
        ThingDef * def = &thingDefs[i + info->startIndex];
        SDL_Rect dst = def->patch.rect;
        dst.w *= info->paletteScale;
        dst.h *= info->paletteScale;
        SDL_RenderCopy(renderer, def->patch.texture, NULL, &dst);

        // Draw selection hover Rect

        SDL_Rect box = dst;
        box.x -= SELECTION_BOX_MARGIN;
        box.y -= SELECTION_BOX_MARGIN;
        box.w += SELECTION_BOX_MARGIN * 2;
        box.h += SELECTION_BOX_MARGIN * 2;

        bool mouseIsOver = SDL_PointInRect(&mouseConverted, &box);

        if ( def->doomedType == thing->type || mouseIsOver )
        {
            if ( def->doomedType == thing->type )
                SetPanelRenderColor(SELECTION_BOX_COLOR);
            else
                SetPanelRenderColor(8);

            DrawRect(&(SDL_FRect){ box.x, box.y, box.w, box.h },
                     SELECTION_BOX_THICKNESS);
        }

        if ( mouseIsOver )
            hoverThingDefIndex = i;
    }

    SDL_RenderSetViewport(renderer, &thingPalette.location);
    // Print highlighted thing name
    if ( hoverThingDefIndex != -1 )
    {
        SetPanelRenderColor(15);
        int i = info->startIndex + hoverThingDefIndex;
        PANEL_RENDER_STRING(2, 1, "%s", thingDefs[i].name);
    }
}

#pragma mark -

void LoadThingPanel(void)
{
    LoadPanelConsole(&thingPanel, PANEL_DATA_DIRECTORY"thing.panel");
    thingPanel.processEvent = ProcessThingPanelEvent;
    thingPanel.render = RenderThingPanel;
    thingPanel.items = items;
    thingPanel.numItems = THP_COUNT;
    thingPanel.selection = 1;

    LoadPanelConsole(&thingCategoryPanel,
              PANEL_DATA_DIRECTORY"thing_category.panel");
    thingCategoryPanel.location.y = 0;
    thingCategoryPanel.processEvent = ProcessThingCategoryPanelEvent;
    thingCategoryPanel.numItems = THING_CATEGORY_COUNT;
    thingCategoryPanel.items = categoryItems;

    // Set up thing category panel items
    for ( int i = 0; i < THING_CATEGORY_COUNT; i++ )
    {
        categoryItems[i].x = 2;
        categoryItems[i].y = i + 2;
        categoryItems[i].width = 20;
    }

    LoadPanelConsole(&thingPalette,
                     PANEL_DATA_DIRECTORY "thing_palette.panel");
    thingPalette.selection = -1; // No items in the thing palette
    thingPalette.render = RenderThingPalette;

    thingPaletteRectOffsets.x = 16;
    thingPaletteRectOffsets.y = 32;
    thingPaletteRectOffsets.w = thingPalette.location.w - 32;
    thingPaletteRectOffsets.h = thingPalette.location.h - 48;
}
