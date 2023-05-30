//
//  line_panel.c
//  de
//
//  Created by Thomas Foster on 5/28/23.
//

#include "panel.h"
#include "panels.h"
#include "map.h"
#include "text.h"
#include "mapview.h"

#include "doomdata.h"

enum
{
    LP_BLOCKS_ALL,
    LP_BLOCKS_MONSTERS,
    LP_BLOCKS_SOUND,
    LP_TWO_SIDED,
    LP_UPPER_UNPEGGED,
    LP_LOWER_UNPEGGED,
    LP_ALWAYS_ON_MAP,
    LP_NOT_ON_MAP,
    LP_SECRET,

    LP_SPECIAL,
    LP_TAG,
    LP_SUGGEST_TAG,

    LP_FRONT,
    LP_BACK,
    LP_LOWER,
    LP_MIDDLE,
    LP_UPPER,
    LP_LOWER_NAME,
    LP_MIDDE_NAME,
    LP_UPPER_NAME,
    LP_OFFSET_X,
    LP_OFFSET_Y,

    LP_NUM_ITEMS,
};

static PanelItem line_panel_items[] =
{
    { 8, 4, 15, -1, 0, -1, LP_FRONT },
    { 8, 5, 15, 0, 0, -1, LP_LOWER },
    { 8, 6, 15, 0, 0, -1, LP_LOWER },
    { 8, 7, 15, 0, 0, -1, LP_LOWER },
    { 8, 8, 15, 0, 0, -1, LP_LOWER },
    { 8, 9, 15, 0, 0, -1, LP_LOWER },
    { 8, 10, 15, 0, 0, -1, LP_LOWER, },
    { 8, 11, 15, 0, 0, -1, LP_LOWER, },
    { 8, 12, 15, 0, 0, -1, LP_LOWER_NAME, },

    { 4, 15, 15, 0, 0, -1, LP_OFFSET_X },
    { 9, 16, 4, 0, -1, -1, LP_SUGGEST_TAG },
    { 15, 16, 7, LP_SPECIAL, -1, LP_TAG, LP_OFFSET_Y }, // suggest

    { 32, 4, 9, -1, LP_LOWER, LP_BLOCKS_ALL, LP_BACK }, // front
    { 46, 4, 9, -1, LP_UPPER, LP_FRONT, -1 }, // back

    { 28, 5, 8, LP_FRONT, LP_LOWER_NAME, LP_BLOCKS_MONSTERS, LP_MIDDLE },
    { 38, 5, 8, LP_BACK, LP_MIDDE_NAME, LP_LOWER, LP_UPPER },
    { 48, 5, 8, LP_BACK, LP_UPPER_NAME, LP_MIDDLE, -1 },

    { 28, 12, 8, LP_LOWER, LP_OFFSET_X, LP_SECRET, LP_MIDDE_NAME },
    { 38, 12, 8, LP_MIDDLE, LP_OFFSET_X, LP_LOWER_NAME, LP_UPPER_NAME },
    { 48, 12, 8, LP_UPPER, LP_OFFSET_X, LP_MIDDE_NAME, -1 },
    { 31, 15, 4, LP_LOWER_NAME, 0, LP_SPECIAL, -1 },
    { 31, 16, 4, 0, -1, LP_SUGGEST_TAG, -1 }
};

Panel line_panel;

void LoadLinePanel(void)
{
    line_panel = LoadPanel(PANEL_DIRECTORY"line.panel");
    line_panel.items = line_panel_items;
    line_panel.numItems = LP_NUM_ITEMS;
}

void RenderMark(const PanelItem * item, int value)
{
    if ( value != 0 ) {
        SetPanelColor(15);
        PrintChar((item->x - 3) * FONT_WIDTH, item->y * FONT_HEIGHT, 7);
    }
}

void RenderLinePanel(const Line * line)
{
//    int windowWidth;
//    int windowHeight;
//    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    SDL_Point midpoint = LineMidpoint(line);
    midpoint = WorldToWindow(&midpoint);

    SDL_Rect panelRect =
    {
        .x = midpoint.x + 32,
        .y = midpoint.y,
//        .x = windowWidth - (line_panel.width * FONT_WIDTH) - FONT_HEIGHT,
//        .y = FONT_HEIGHT,
        .w = line_panel.width * FONT_WIDTH,
        .h = line_panel.height * FONT_HEIGHT
    };

    PanelRenderTexture(&line_panel, panelRect.x, panelRect.y);

    SDL_RenderSetViewport(renderer, &panelRect);

    // Render current selection.

    PanelItem item = line_panel.items[line_panel.selection];
    for ( int x = item.x; x < item.x + item.width; x++ )
    {
        int y = item.y;
        int i = y * line_panel.width + x;
        BufferCell cell = GetCell(line_panel.data[i]);

        SDL_Rect r = {
            x * FONT_WIDTH,
            item.y * FONT_HEIGHT,
            FONT_WIDTH,
            FONT_HEIGHT
        };

        SetPanelColor(7);
        SDL_RenderFillRect(renderer, &r);

        SetPanelColor(cell.foreground);
        PrintChar(x * FONT_WIDTH, y * FONT_HEIGHT, cell.character);
    }

    PanelItem * items = line_panel_items;
    RenderMark(&items[LP_BLOCKS_ALL], line->flags & ML_BLOCKING);
    RenderMark(&items[LP_BLOCKS_MONSTERS], line->flags & ML_BLOCKMONSTERS);
    RenderMark(&items[LP_BLOCKS_SOUND], line->flags & ML_SOUNDBLOCK);
    RenderMark(&items[LP_TWO_SIDED], line->flags & ML_TWOSIDED);
    RenderMark(&items[LP_UPPER_UNPEGGED], line->flags & ML_DONTPEGTOP);
    RenderMark(&items[LP_LOWER_UNPEGGED], line->flags & ML_DONTPEGBOTTOM);
    RenderMark(&items[LP_ALWAYS_ON_MAP], line->flags & ML_MAPPED);
    RenderMark(&items[LP_NOT_ON_MAP], line->flags & ML_DONTDRAW);
    RenderMark(&items[LP_SECRET], line->flags & ML_SECRET);

    if ( line->panelBackSelected ) {
        RenderMark(&items[LP_BACK], 1);
    } else {
        RenderMark(&items[LP_FRONT], 1);
    }

    SDL_RenderSetViewport(renderer, NULL);
}

static void HandleReturnPressed(Line * line, int selection)
{
    switch ( selection )
    {
        case LP_BLOCKS_ALL:       line->flags ^= ML_BLOCKING; break;
        case LP_BLOCKS_MONSTERS:  line->flags ^= ML_BLOCKMONSTERS; break;
        case LP_BLOCKS_SOUND:     line->flags ^= ML_SOUNDBLOCK; break;
        case LP_TWO_SIDED:        line->flags ^= ML_TWOSIDED; break;
        case LP_UPPER_UNPEGGED:   line->flags ^= ML_DONTPEGTOP; break;
        case LP_LOWER_UNPEGGED:   line->flags ^= ML_DONTPEGBOTTOM; break;
        case LP_ALWAYS_ON_MAP:    line->flags ^= ML_MAPPED; break;
        case LP_NOT_ON_MAP:       line->flags ^= ML_DONTDRAW; break;
        case LP_SECRET:           line->flags ^= ML_SECRET; break;

        case LP_FRONT:  line->panelBackSelected = false; break;
        case LP_BACK:   line->panelBackSelected = true; break;

        default:
            break;
    }
}

bool HandleLinePanelEvent(const SDL_Event * event, void * data)
{
    Line * line = data;

    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_RETURN:
                    HandleReturnPressed(line, line_panel.selection);
                    return true;

                default:
                    return false;
            }
            break;

        default:
            return false;
    }
}
