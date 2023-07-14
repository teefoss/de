//
//  line_panel.c
//  de
//
//  Created by Thomas Foster on 5/28/23.
//

#include "p_line_panel.h"
#include "m_map.h"
#include "text.h"
#include "e_map_view.h"
#include "patch.h"
#include "p_texture_panel.h"

#include "doomdata.h"

#define NUM_SPECIAL_CATEGORIES 7
#define NUM_SPECIAL_ROWS 14

enum
{
    LP_FRONT = 1,
    LP_BACK,

    LP_UPPER,
    LP_MIDDLE,
    LP_LOWER,

    LP_OFFSET_X,
    LP_OFFSET_Y,

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

    LP_NUM_ITEMS,
};

static PanelItem linePanelItems[] =
{
    { 0 },
    { 17, 2, 5, true, 13, 2, 21, 2 }, // front
    { 27, 2, 4, true, 23, 2, 30, 2 }, // back

    { 15, 6,  8, true, 2, 4, 13, 9 },
    { 15, 13, 8, true, 2, 11, 13, 16 },
    { 15, 20, 8, true, 2, 18, 13, 23 },

    { 12, 25, 4, true, 2, 25, 15, 25 }, // offset x
    { 27, 25, 4, true, 17, 25, 30, 25 }, // offset y

    { 6, 27, 27, true, 2, 27, 34, 27 }, // LP_BLOCKS_ALL,
    { 6, 28, 27, true, 2, 28, 34, 28 },
    { 6, 29, 27, true, 2, 29, 34, 29 },
    { 6, 30, 27, true, 2, 30, 34, 30 },
    { 6, 31, 27, true, 2, 31, 34, 31 },
    { 6, 32, 27, true, 2, 32, 34, 32 },
    { 6, 33, 27, true, 2, 33, 34, 33 },
    { 6, 34, 27, true, 2, 34, 34, 34 },
    { 6, 35, 27, true, 2, 35, 34, 35 },

    { 2, 37, 31 }, // special
    { 7, 38, 4, true, 2, 38, 20, 38 }, // tag
    { 17, 38, 7 }, // suggest
};

static PanelItem specialCategoryItems[] =
{
    { 2, 2, 15 }, // None
    { 2, 4, 15 }, // Button
    { 2, 5, 15 },
    { 2, 6, 15 },
    { 2, 7, 15 },
    { 2, 8, 15 },
    { 2, 9, 15 },
    { 2, 10, 15 }
};

static PanelItem specialItems[NUM_SPECIAL_ROWS]; // Inited in LoadLinePanels()

Panel linePanel;
Panel lineSpecialCategoryPanel;
Panel lineSpecialPanel;

// When a line is selected, it is copied here. When any property is changed,
// this updates the base, then base is copied to all other selected lines.
static Line base;
int flagToToggle;

LineSpecial * specials;
int numSpecials;

typedef struct
{
    char name[80];
    int startIndex;
    int count;
    int maxShortNameLength;
} SpecialCategory;

SpecialCategory categories[NUM_SPECIAL_CATEGORIES];
int numCategories = 0;
int selectedCategory; // Index into `categories`
int selectedSpecial;
int firstSpecial; // Index of topmost visible special in list.

//static void RenderSpecialsPanel(void);
static bool ProcessSpecialPanelEvent(const SDL_Event * event);

void OpenLinePanel(Line * line)
{
    base = *line;

    OpenPanel(&linePanel, NULL);
    UpdateLinePanelContent();
}

void SetFlag(Line * line, int flags, int flag)
{
    line->flags &= ~flag;
    line->flags |= flags & flag;
}

/// Called after a property was changed, propogating the change it to all
/// selected lines.
void LinePanelApplyChange(LineProperty property)
{
    int s = base.panelBackSelected;

    for ( int i = 0; i < map.lines->count; i++ )
    {
        Line * line = Get(map.lines, i);

        if ( line->deleted )
            continue;

        if ( line->selected )
        {
            switch ( property )
            {
                case LINE_BLOCKS_ALL:
                case LINE_BLOCKS_MONSTERS:
                case LINE_TWO_SIDED:
                case LINE_TOP_UNPEGGED:
                case LINE_BOTTOM_UNPEGGED:
                case LINE_SECRET:
                case LINE_BLOCKS_SOUND:
                case LINE_DONT_DRAW:
                case LINE_ALWAYS_DRAW:
                {
                    int flag = 1 << property;
                    line->flags &= ~flag;
                    line->flags |= base.flags & flag;
                    break;
                }
                case LINE_SPECIAL:
                    line->special = base.special;
                    break;
                case LINE_TAG:
                    line->tag = base.tag;
                    break;
                case LINE_OFFSET_X:
                    line->sides[s].offsetX = base.sides[s].offsetX;
                    break;
                case LINE_OFFSET_Y:
                    line->sides[s].offsetY = base.sides[s].offsetY;
                    break;
                case LINE_TOP_TEXTURE:
                    strncpy(line->sides[s].top, base.sides[s].top, 8);
                    break;
                case LINE_MIDDLE_TEXTURE:
                    strncpy(line->sides[s].middle, base.sides[s].middle, 8);
                    break;
                case LINE_BOTTOM_TEXTURE:
                    strncpy(line->sides[s].bottom, base.sides[s].bottom, 8);
                    break;
                case LINE_SIDE_SELECTION:
                    line->panelBackSelected = base.panelBackSelected;
                    break;
            }
        }
    }
}


#pragma mark -

static void LoadSpecials(const char * path)
{
//    const char * path = "doom_dsp/linespecials.dsp";
    FILE * file = fopen(path, "r");
    if ( file == NULL ) {
        fprintf(stderr, "Error: could not find '%s'!\n", path);
        return;
    }

    fscanf(file, "numspecials: %d\n", &numSpecials);
    specials = calloc(numSpecials, sizeof(*specials));

    int longestName = 0;

    for ( int i = 0; i < numSpecials; i++ )
    {
        int id;
        fscanf(file, "%d:", &id);
        specials[i].id = id;

        fscanf(file, "%s\n", specials[i].name);

        int len = (int)strlen(specials[i].name);
        if ( len > longestName )
            longestName = len;

        // Extract the category name.
        char category[80] = { 0 };
        strncpy(category, specials[i].name, 80);
        long firstSpaceIndex = strchr(category, '_') - category;
        specials[i].shortName = &specials[i].name[firstSpaceIndex + 1];

        category[firstSpaceIndex] = '\0';

        if ( numCategories == 0 ||
            (numCategories > 0
             && strncmp(categories[numCategories - 1].name, category, 80) != 0) )
        {
            // Found a new category.
            strncpy(categories[numCategories].name, category, 80);
            categories[numCategories].startIndex = i;
            categories[numCategories].count++;
            numCategories++;
        }
        else
        {
            categories[numCategories - 1].count++;
        }

        // Make the display name more purdy.
        for ( char * c = specials[i].name; *c; c++ )
            if ( *c == '_' )
                *c = ' ';
    }

    for ( int i = 0; i < numCategories; i++ )
    {
        SpecialCategory * category = &categories[i];
        category->maxShortNameLength = 0;

        int start = category->startIndex;
        int end = start + category->count - 1;

        for ( int j = start; j <= end; j++ )
        {
            int len = (int)strlen(specials[j].shortName);
            if (  len > category->maxShortNameLength )
                category->maxShortNameLength = len;
        }

#if 0
        printf("Cateogry %d: %s; start: %d; count: %d\n",
               i,
               categories[i].name,
               categories[i].startIndex,
               categories[i].count);
#endif
    }
    printf("Longest name: %d\n", longestName);
}

LineSpecial * FindSpecial(int id)
{
    for ( int i = 0; i < numSpecials; i++ )
        if ( specials[i].id == id )
            return &specials[i];
    
    return NULL;
}

void UpdateVisibleSpecials(void)
{
    SpecialCategory * cat = &categories[selectedCategory];

    int last = cat->startIndex + cat->count - 1;

    int specialIndex = cat->startIndex + firstSpecial;

    for ( int i = 0; i < NUM_SPECIAL_ROWS; i++ )
    {
        PanelItem * item = &specialItems[i];

        for ( int x = item->x - 2; x < item->x + item->width; x++ )
            PanelPrint(&lineSpecialPanel, x, item->y, " "); // Clear line.

        if ( specialIndex <= last )
        {
            if ( ((Line *)linePanel.data)->special == specials[specialIndex].id )
                UpdatePanelConsole(&lineSpecialPanel,
                            specialItems[i].x - 2,
                            specialItems[i].y, 7, true);

            PanelPrint(&lineSpecialPanel,
                       specialItems[i].x,
                       specialItems[i].y,
                       specials[specialIndex].shortName);

            specialIndex++;
        }
    }
}

int SuggestTag(void)
{
    Line * line = map.lines->data;
    int max = -1;
    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        if ( line->tag > max )
            max = line->tag;
    }

    return max + 1;
}

#pragma mark - INPUT

static void TextEditingCompletionHandler(void)
{
    switch ( linePanel.textItem )
    {
        case LP_TAG:
            LinePanelApplyChange(LINE_TAG);
            break;
        case LP_OFFSET_X:
            LinePanelApplyChange(LINE_OFFSET_X);
            break;
        case LP_OFFSET_Y:
            LinePanelApplyChange(LINE_OFFSET_Y);
            break;
        default:
            break;
    }
}

static void LinePanelAction(int selection)
{
    Side * side = &base.sides[base.panelBackSelected];

    switch ( selection )
    {
        case LP_LOWER:
            OpenTexturePanel(side->bottom, LINE_BOTTOM_TEXTURE);
            break;
        case LP_MIDDLE:
            OpenTexturePanel(side->middle, LINE_MIDDLE_TEXTURE);
            break;
        case LP_UPPER:
            OpenTexturePanel(side->top, LINE_TOP_TEXTURE);
            break;
        case LP_BLOCKS_ALL:
            base.flags ^= ML_BLOCKING;
            LinePanelApplyChange(LINE_BLOCKS_ALL);
            break;
        case LP_BLOCKS_MONSTERS:
            base.flags ^= ML_BLOCKMONSTERS;
            LinePanelApplyChange(LINE_BLOCKS_MONSTERS);
            break;
        case LP_TWO_SIDED:
            base.flags ^= ML_TWOSIDED;
            LinePanelApplyChange(LINE_TWO_SIDED);
            break;
        case LP_UPPER_UNPEGGED:
            base.flags ^= ML_DONTPEGTOP;
            LinePanelApplyChange(LINE_TOP_UNPEGGED);
            break;
        case LP_LOWER_UNPEGGED:
            base.flags ^= ML_DONTPEGBOTTOM;
            LinePanelApplyChange(LINE_BOTTOM_UNPEGGED);
            break;
        case LP_SECRET:
            base.flags ^= ML_SECRET;
            LinePanelApplyChange(LINE_SECRET);
            break;
        case LP_BLOCKS_SOUND:
            base.flags ^= ML_SOUNDBLOCK;
            LinePanelApplyChange(LINE_BLOCKS_SOUND);
            break;
        case LP_NOT_ON_MAP:
            base.flags ^= ML_DONTDRAW;
            LinePanelApplyChange(LINE_DONT_DRAW);
            break;
        case LP_ALWAYS_ON_MAP:
            base.flags ^= ML_MAPPED;
            LinePanelApplyChange(LINE_ALWAYS_DRAW);
            break;
        case LP_FRONT:
            base.panelBackSelected = false;
            LinePanelApplyChange(LINE_SIDE_SELECTION);
            break;
        case LP_BACK:
            if ( base.flags & ML_TWOSIDED )
            {
                base.panelBackSelected = true;
                LinePanelApplyChange(LINE_SIDE_SELECTION);
            }
            break;
        case LP_SPECIAL:
            OpenPanel(&lineSpecialCategoryPanel, NULL);
            break;
        case LP_TAG:
            StartTextEditing(&linePanel, LP_TAG, &base.tag, VALUE_INT);
            break;
        case LP_OFFSET_X:
            StartTextEditing(&linePanel, LP_OFFSET_X, &side->offsetX, VALUE_INT);
            break;
        case LP_OFFSET_Y:
            StartTextEditing(&linePanel, LP_OFFSET_Y, &side->offsetY, VALUE_INT);
            break;
        case LP_SUGGEST_TAG:
            base.tag = SuggestTag();
            LinePanelApplyChange(LINE_TAG);
            break;
        default:
            break;
    }
}

static bool ProcessLinePanelEvent(const SDL_Event * event)
{
    switch ( event->type )
    {
        case SDL_MOUSEBUTTONDOWN:
            switch ( event->button.button )
            {
                case SDL_BUTTON_LEFT:
                    if (   linePanel.mouseLocation.x == -1
                        || linePanel.mouseLocation.y == -1 )
                        return false; // Mouse click was outside panel

                    LinePanelAction(linePanel.selection);
                    return true;
                default:
                    return false;
            }
            break;

        default:
            return false;
    }
}

void OpenSpecialsPanel(void)
{
    // TODO: don't remake entire panel, change to SetupPanel(&panel)
    selectedCategory = lineSpecialCategoryPanel.selection - 1;
    SpecialCategory * cat = &categories[selectedCategory];

    FreePanel(&lineSpecialPanel);

    lineSpecialPanel = NewPanel(lineSpecialCategoryPanel.width,
                             lineSpecialCategoryPanel.selection + 2,
                             cat->maxShortNameLength + 4,
                             cat->count + 2,
                             cat->count);


    lineSpecialPanel.location.y = 0;

    SetPanelColor(10, 1);
    for ( int y = 0; y < lineSpecialPanel.height; y++ )
    {
        for ( int x = 0; x < lineSpecialPanel.width; x++ )
        {
            PanelPrint(&lineSpecialPanel, x, y, " ");
        }
    }

    for ( int i = 0; i < cat->count; i++ )
    {
        PanelItem * item = &lineSpecialPanel.items[i];
        item->x = 2;
        item->y = i + 1;
        item->width = cat->maxShortNameLength;

        SetPanelColor(15, 1);
        PanelPrint(&lineSpecialPanel,
                   item->x,
                   item->y,
                   specials[i + cat->startIndex].shortName);
    }

//    specialsPanel.render = RenderSpecialsPanel;
    lineSpecialPanel.processEvent = ProcessSpecialPanelEvent;

//    rightPanels[++topPanel] = &lineSpecialPanel;
    OpenPanel(&lineSpecialPanel, NULL);
}

static bool ProcessSpecialCategoriesPanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event, &lineSpecialCategoryPanel) )
    {
        if ( lineSpecialCategoryPanel.selection == 0 )
        {
            ((Line *)linePanel.data)->special = 0;
            topPanel--;
        }
        else
        {
            // If the special panel is already open, close it first.
            if ( GetPanelStackPosition(&lineSpecialPanel) < topPanel )
                topPanel--;

            OpenSpecialsPanel();
        }

        return true;
    }

    return false;
}

static bool ProcessSpecialPanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event, &lineSpecialPanel) )
    {
        int i = categories[selectedCategory].startIndex;
        i += firstSpecial;
        i += lineSpecialPanel.selection;

        ((Line *)linePanel.data)->special = specials[i].id;
        topPanel -= 1;
        return true;
    }

    return false;
}

void ItemPrint(const Panel * panel, int item, const char * string)
{
    PanelPrint(panel, panel->items[item].x, panel->items[item].y, string);

    // Clear remainder of item's space.
    int start = panel->items[item].x + (int)strlen(string);
    int end = panel->items[item].x + panel->items[item].width - 1;

    for ( int x = start; x <= end; x++ )
        PanelPrint(panel, x, panel->items[item].y, " ");
}

void UpdateLinePanelContent(void)
{
    Side * side = &base.sides[base.panelBackSelected];

    int numSelected = 0;
    int index = -1;
    for ( int i = 0; i < map.lines->count; i++ )
    {
        Line * check = Get(map.lines, i);
        if ( check->selected )
        {
            index = i;
            numSelected++;
        }
    }

    char title[100] = { 0 };
    if ( numSelected == 1 )
    {
        snprintf(title, sizeof(title), "Line %d", index);
        SetPanelColor(15, 1);
    }
    else if ( numSelected > 1 )
    {
        snprintf(title, sizeof(title), "%d Lines Selected", numSelected);
        SetPanelColor(12, 1);
    }

    PanelPrint(&linePanel, 2, 1, "                   ");
    PanelPrint(&linePanel, 2, 1, title);

    SetPanelColor(11, 1);

    ItemPrint(&linePanel, LP_LOWER, side->bottom);
    ItemPrint(&linePanel, LP_MIDDLE, side->middle);
    ItemPrint(&linePanel, LP_UPPER, side->top);

    // Gray out back side as needed.

    {
        int x = linePanelItems[LP_BACK].x;
        int y = linePanelItems[LP_BACK].y;

        if ( base.flags & ML_TWOSIDED )
        {
            SetPanelColor(11, 1);
            PanelPrint(&linePanel, x - 4, y, "( )");
            SetPanelColor(15, 1);
            PanelPrint(&linePanel, x, y, "Back");
        }
        else
        {
            SetPanelColor(8, 1);
            PanelPrint(&linePanel, x - 4, y, "( ) Back");
        }
    }
}

#pragma mark - RENDER

static void RenderLinePanel(void)
{
    const Line * line = &base;
    const Side * side = &line->sides[line->panelBackSelected];
    PanelItem * items = linePanelItems;

    //
    // Upper, Middle, and Lower Textures
    //

    SDL_Rect textureRect = { .w = 12 * FONT_WIDTH, .h = 6 * FONT_HEIGHT };

    textureRect.x = items[LP_UPPER].mouseX1 * FONT_WIDTH;
    textureRect.y = items[LP_UPPER].mouseY1 * FONT_HEIGHT;
    if ( side->top[0] != '-')
        RenderTextureInRect(side->top, &textureRect);

    textureRect.y = items[LP_MIDDLE].mouseY1 * FONT_HEIGHT;
    if ( side->middle[0] != '-')
        RenderTextureInRect(side->middle, &textureRect);

    textureRect.y = items[LP_LOWER].mouseY1 * FONT_HEIGHT;
    if ( side->bottom[0] != '-')
        RenderTextureInRect(side->bottom, &textureRect);

    //
    // Option marks
    //

    RenderMark(&items[LP_BLOCKS_ALL], line->flags & ML_BLOCKING);
    RenderMark(&items[LP_BLOCKS_MONSTERS], line->flags & ML_BLOCKMONSTERS);
    RenderMark(&items[LP_BLOCKS_SOUND], line->flags & ML_SOUNDBLOCK);
    RenderMark(&items[LP_TWO_SIDED], line->flags & ML_TWOSIDED);
    RenderMark(&items[LP_UPPER_UNPEGGED], line->flags & ML_DONTPEGTOP);
    RenderMark(&items[LP_LOWER_UNPEGGED], line->flags & ML_DONTPEGBOTTOM);
    RenderMark(&items[LP_ALWAYS_ON_MAP], line->flags & ML_MAPPED);
    RenderMark(&items[LP_NOT_ON_MAP], line->flags & ML_DONTDRAW);
    RenderMark(&items[LP_SECRET], line->flags & ML_SECRET);

    //
    // Front/Back selection mark
    //

    if ( line->panelBackSelected ) {
        RenderMark(&items[LP_BACK], 1);
    } else {
        RenderMark(&items[LP_FRONT], 1);
    }

    //
    // X and Y Offsets
    //

    if ( !linePanel.isTextEditing || (linePanel.isTextEditing && linePanel.textItem != LP_OFFSET_X) )
    {
        PANEL_RENDER_STRING(items[LP_OFFSET_X].x, items[LP_OFFSET_X].y, "%d", side->offsetX);
    }

    if ( !linePanel.isTextEditing || (linePanel.isTextEditing && linePanel.textItem != LP_OFFSET_Y) )
    {
        PANEL_RENDER_STRING(items[LP_OFFSET_Y].x, items[LP_OFFSET_Y].y, "%d", side->offsetY);
    }

    //
    // Special
    //

    int x = items[LP_SPECIAL].x;
    int y = items[LP_SPECIAL].y;
    SetPanelRenderColor(11);

    if ( line->special == 0 )
    {
        PANEL_RENDER_STRING(x, y, "(none)");
    }
    else
    {
        LineSpecial * special = FindSpecial(line->special);

        if ( special == NULL )
        {
            SetPanelRenderColor(12);
            PANEL_RENDER_STRING(x, y, "Bad Special! (%d)", line->special);
        }
        else
            PANEL_RENDER_STRING(x, y, "%s", special->name);
    }

    //
    // Tag
    //

    if ( !linePanel.isTextEditing || (linePanel.isTextEditing && linePanel.textItem != LP_TAG) )
    {
        SetPanelRenderColor(15);
        PANEL_RENDER_STRING(items[LP_TAG].x, items[LP_TAG].y, "%d", line->tag);
    }

//    SDL_RenderSetViewport(renderer, NULL);
}

//static void RenderSpecialCategoriesPanel(void)
//{
//    RenderPanelTexture(&specialCategoriesPanel);
//    RenderPanelSelection(&specialCategoriesPanel);
//}

//static void RenderSpecialsPanel(void)
//{
//    RenderPanelTexture(&specialsPanel);
//
//    SDL_Rect panelLocation = PanelRenderLocation(&specialsPanel);
//    SDL_RenderSetViewport(renderer, &panelLocation);
//
//    RenderPanelSelection(&specialsPanel);
//
//    SDL_RenderSetViewport(renderer, NULL);
//}

#pragma mark -

void LoadLinePanels(const char * dspPath)
{
    linePanel = LoadPanel(PANEL_DATA_DIRECTORY "line.panel");
//    linePanel.location.x = PANEL_SCREEN_MARGIN;
    linePanel.location.y = 0;
    linePanel.items = linePanelItems;
    linePanel.numItems = LP_NUM_ITEMS;
    linePanel.render = RenderLinePanel;
    linePanel.processEvent = ProcessLinePanelEvent;
    linePanel.selection = 1;
    linePanel.textEditingCompletionHandler = TextEditingCompletionHandler;

    printf("Loading line specials from %s...\n", dspPath);
    LoadSpecials(dspPath);

    lineSpecialCategoryPanel = LoadPanel(PANEL_DATA_DIRECTORY"special_categories.panel");
    lineSpecialCategoryPanel.items = specialCategoryItems;
    lineSpecialCategoryPanel.numItems = numCategories + 1;
//    specialCategoriesPanel.parent = &linePanel;
//    specialCategoriesPanel.location.x = 0;
    lineSpecialCategoryPanel.location.y = 0;
    lineSpecialCategoryPanel.render = NULL;
    lineSpecialCategoryPanel.processEvent = ProcessSpecialCategoriesPanelEvent;

    if ( numCategories != NUM_SPECIAL_CATEGORIES )
        printf("!!! warning: numCategories != NUM_SPECIAL_CATEGORIES\n");
}

void FreeLinePanels(void)
{
    FreePanel(&linePanel);
    FreePanel(&lineSpecialCategoryPanel);
    FreePanel(&lineSpecialPanel);
}
