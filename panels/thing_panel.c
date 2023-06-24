//
//  thing_panel.c
//  de
//
//  Created by Thomas Foster on 6/19/23.
//

#include "thing_panel.h"
#include "doomdata.h"
#include "patch.h"
#include "map.h"

Panel thingPanel;
Panel thingCategoryPanel;
Panel thingPalette;

static int category; // Selected category index.
SDL_Rect thingPaletteRect; // The rect where things are displayed.

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
    [THP_TYPE] = { 2, 10, 25, THP_SW, THP_EASY, -1, -1, true, 2, 3, 13, 8 },
    [THP_NW] = { 16, 4, 2, -1, THP_W, THP_TYPE, THP_N },
    [THP_N] = { 20, 4, 2, -1, THP_S, THP_NW, THP_NE },
    [THP_NE] = { 24, 4, 2, -1, THP_E, THP_N, -1 },
    [THP_W] = { 16, 6, 2, THP_NW, THP_SW, THP_TYPE, THP_E },
    [THP_E] = { 24, 6, 2, THP_NE, THP_SE, THP_W, -1 },
    [THP_SW] = { 16, 8, 2, THP_W, THP_TYPE, -1, THP_S },
    [THP_S] = { 20, 8, 2, THP_N, THP_TYPE, THP_SW, THP_SE },
    [THP_SE] = { 24, 8, 2, THP_E, THP_TYPE, THP_S, -1 },

    [THP_EASY] = { 6, 12, 7, THP_TYPE, THP_NORMAL, -1, THP_AMBUSH },
    [THP_NORMAL] = { 6, 13, 7, THP_EASY, THP_HARD, -1, THP_NETWORK },
    [THP_HARD] = { 6, 14, 7, THP_NORMAL, -1, -1, THP_NETWORK },
    [THP_AMBUSH] = { 18, 12, 7, THP_TYPE, THP_NETWORK, THP_EASY, -1 },
    [THP_NETWORK] = { 18, 13, 7, THP_AMBUSH, -1, THP_NORMAL, -1 },
};

static PanelItem categoryItems[THING_CATEGORY_COUNT];

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
    PANEL_RENDER_STRING(16, 3, "%s", directionNames[itemIndex]);
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

            DrawRect(&box, SELECTION_BOX_THICKNESS);
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
    thingPanel = LoadPanel(PANEL_DATA_DIRECTORY"thing.panel");
    thingPanel.location.x = PANEL_SCREEN_MARGIN;
    thingPanel.location.y = PANEL_SCREEN_MARGIN;
    thingPanel.processEvent = ProcessThingPanelEvent;
    thingPanel.render = RenderThingPanel;
    thingPanel.items = items;
    thingPanel.numItems = THP_COUNT;
    thingPanel.selection = 1;

    thingCategoryPanel = LoadPanel(PANEL_DATA_DIRECTORY"thing_category.panel");
    thingCategoryPanel.location.x
        = thingPanel.location.x + items[THP_TYPE].x * FONT_WIDTH;
    thingCategoryPanel.location.y
        = thingPanel.location.y + (items[THP_TYPE].y + 1) * FONT_HEIGHT;
    thingCategoryPanel.processEvent = ProcessThingCategoryPanelEvent;
    thingCategoryPanel.numItems = THING_CATEGORY_COUNT;
    thingCategoryPanel.items = categoryItems;

    // Set up thing category panel items
    for ( int i = 0; i < THING_CATEGORY_COUNT; i++ )
    {
        categoryItems[i].x = 2;
        categoryItems[i].y = i + 1;
        categoryItems[i].width = 17;
        categoryItems[i].left = -1;
        categoryItems[i].right = -1;
        categoryItems[i].up = i - 1;
        categoryItems[i].down = i + 1;
    }

    categoryItems[0].up = THING_CATEGORY_COUNT - 1;
    categoryItems[THING_CATEGORY_COUNT - 1].down = 0;

    thingPalette = LoadPanel(PANEL_DATA_DIRECTORY "thing_palette.panel");
    thingPalette.location.x = 100;
    thingPalette.location.y = 100;
    thingPalette.selection = -1; // No items in the thing palette
    thingPalette.render = RenderThingPalette;
    thingPalette.selection = -1;

    thingPaletteRect.x = thingPalette.location.x + 16;
    thingPaletteRect.y = thingPalette.location.y + 32;
    thingPaletteRect.w = (thingPalette.location.w - 32);
    thingPaletteRect.h = thingPalette.location.h - 48;
}
