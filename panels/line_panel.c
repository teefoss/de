//
//  line_panel.c
//  de
//
//  Created by Thomas Foster on 5/28/23.
//

#include "line_panel.h"
#include "map.h"
#include "text.h"
#include "mapview.h"
#include "patch.h"
#include "texture_panel.h"

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
    { 17, 2, 5, -1, LP_UPPER, -1, LP_BACK, true, 13, 2, 21, 2 }, // front
    { 27, 2, 4, -1, LP_UPPER, LP_FRONT, -1, true, 23, 2, 30, 2 }, // back

    { 15, 6,  8, LP_FRONT, 0, -1, -1, true, 2, 4, 13, 9 },
    { 15, 13, 8, 0, 0, -1, -1, true, 2, 11, 13, 16 },
    { 15, 20, 8, 0, 0, -1, -1, true, 2, 18, 13, 23 },

    { 12, 25, 4, 0, LP_BLOCKS_ALL, -1, LP_OFFSET_Y, true, 2, 25, 15, 25 }, // offset x
    { 27, 25, 4, LP_LOWER, LP_BLOCKS_ALL, LP_OFFSET_X, -1, true, 17, 25, 30, 25 }, // offset y

    { 6, 27, 27, LP_OFFSET_X, 0, -1, -1, true, 2, 27, 34, 27 }, // LP_BLOCKS_ALL,
    { 6, 28, 27, 0, 0, -1, -1, true, 2, 28, 34, 28 },
    { 6, 29, 27, 0, 0, -1, -1, true, 2, 29, 34, 29 },
    { 6, 30, 27, 0, 0, -1, -1, true, 2, 30, 34, 30 },
    { 6, 31, 27, 0, 0, -1, -1, true, 2, 31, 34, 31 },
    { 6, 32, 27, 0, 0, -1, -1, true, 2, 32, 34, 32 },
    { 6, 33, 27, 0, 0, -1, -1, true, 2, 33, 34, 33 },
    { 6, 34, 27, 0, 0, -1, -1, true, 2, 34, 34, 34 },
    { 6, 35, 27, 0, 0, -1, -1, true, 2, 35, 34, 35 },

    { 2, 37, 31, 0, 0, -1, -1 }, // special
    { 7, 38, 4, 0, -1, -1, LP_SUGGEST_TAG, true, 2, 38, 20, 38 }, // tag
    { 17, 38, 7, LP_SPECIAL, -1, LP_TAG, -1 }, // suggest
};

static PanelItem specialCategoryItems[] =
{
    { 2, 1, 11, -1, 0, -1, -1 }, // None
    { 2, 3, 11, 0, 0, -1, -1 }, // Button
    { 2, 4, 11, 0, 0, -1, -1 },
    { 2, 5, 11, 0, 0, -1, -1 },
    { 2, 6, 11, 0, 0, -1, -1 },
    { 2, 7, 11, 0, 0, -1, -1 },
    { 2, 8, 11, 0, 0, -1, -1 },
    { 2, 9, 11, 0, -1, -1, -1 }
};

static PanelItem specialItems[NUM_SPECIAL_ROWS]; // Inited in LoadLinePanels()

Panel linePanel;
Panel specialCategoriesPanel;
Panel specialsPanel;

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
            PanelPrint(&specialsPanel, x, item->y, " "); // Clear line.

        if ( specialIndex <= last )
        {
            if ( ((Line *)linePanel.data)->special == specials[specialIndex].id )
                UpdatePanelConsole(&specialsPanel,
                            specialItems[i].x - 2,
                            specialItems[i].y, 7, true);

            PanelPrint(&specialsPanel,
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

static void LinePanelAction(int selection)
{
    Line * line = linePanel.data;
    Side * side = &line->sides[line->panelBackSelected];

    switch ( selection )
    {
        case LP_LOWER:
            OpenTexturePanel(side->bottom);
            break;
        case LP_MIDDLE:
            OpenTexturePanel(side->middle);
            break;
        case LP_UPPER:
            OpenTexturePanel(side->top);
            break;

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

        case LP_SPECIAL:
            openPanels[++topPanel] = &specialCategoriesPanel;
            break;

        case LP_TAG:
            StartTextEditing(&linePanel, LP_TAG, &line->tag, VALUE_INT);
            break;

        case LP_OFFSET_X:
            StartTextEditing(&linePanel, LP_OFFSET_X, &side->offsetX, VALUE_INT);
            break;

        case LP_OFFSET_Y:
            StartTextEditing(&linePanel, LP_OFFSET_Y, &side->offsetY, VALUE_INT);
            break;

        case LP_SUGGEST_TAG:
            line->tag = SuggestTag();
            break;

        default:
            break;
    }
}

static bool ProcessLinePanelEvent(const SDL_Event * event)
{
    switch ( event->type )
    {
        case SDL_KEYDOWN:
            switch ( event->key.keysym.sym )
            {
                case SDLK_RETURN:
                    LinePanelAction(linePanel.selection);
                    return true;

                default:
                    return false;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            switch ( event->button.button )
            {
                case SDL_BUTTON_LEFT:
                    if ( linePanel.mouseItem != -1 )
                    {
                        LinePanelAction(linePanel.selection);
                        return true;
                    }

                    return false;
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
    selectedCategory = specialCategoriesPanel.selection - 1;
    SpecialCategory * cat = &categories[selectedCategory];

    FreePanel(&specialsPanel);

    specialsPanel = NewPanel(specialCategoriesPanel.width,
                             specialCategoriesPanel.selection + 2,
                             cat->maxShortNameLength + 4,
                             cat->count + 2,
                             cat->count);


    SDL_Rect rect = specialCategoriesPanel.location;

    specialsPanel.location.x = rect.x + rect.w + 8;
    specialsPanel.location.y = rect.y + rect.h - specialsPanel.location.h;
    specialsPanel.items[0].up = -1;
    specialsPanel.items[cat->count - 1].down = -1;

    SetPanelColor(10, 1);
    for ( int y = 0; y < specialsPanel.height; y++ )
    {
        for ( int x = 0; x < specialsPanel.width; x++ )
        {
            PanelPrint(&specialsPanel, x, y, " ");
        }
    }

    for ( int i = 0; i < cat->count; i++ )
    {
        PanelItem * item = &specialsPanel.items[i];
        item->x = 2;
        item->y = i + 1;
        item->width = cat->maxShortNameLength;
        item->left = -1;
        item->right = -1;

        SetPanelColor(15, 1);
        PanelPrint(&specialsPanel,
                   item->x,
                   item->y,
                   specials[i + cat->startIndex].shortName);
    }

//    specialsPanel.render = RenderSpecialsPanel;
    specialsPanel.processEvent = ProcessSpecialPanelEvent;

    openPanels[++topPanel] = &specialsPanel;
}

static bool ProcessSpecialCategoriesPanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event, &specialCategoriesPanel) )
    {
        if ( specialCategoriesPanel.selection == 0 )
        {
            ((Line *)linePanel.data)->special = 0;
            topPanel--;
        }
        else
        {
            // If the special panel is already open, close it first.
            if ( GetPanelStackPosition(&specialsPanel) < topPanel )
                topPanel--;

            OpenSpecialsPanel();
        }

        return true;
    }

    return false;
}

static bool ProcessSpecialPanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event, &specialsPanel) )
    {
        int i = categories[selectedCategory].startIndex;
        i += firstSpecial;
        i += specialsPanel.selection;

        ((Line *)linePanel.data)->special = specials[i].id;
        topPanel -= 2;
        return true;
    }

    if ( event->type == SDL_KEYDOWN )
    {
        SpecialCategory * cat = &categories[selectedCategory];

        switch ( event->key.keysym.sym )
        {
            case SDLK_DOWN:
                if ( specialsPanel.selection > NUM_SPECIAL_ROWS / 2
                    && specialsPanel.selection + NUM_SPECIAL_ROWS < cat->count )
                {
                    firstSpecial++;
                    UpdateVisibleSpecials();
                }
                else
                {
                    specialsPanel.selection++;
                }

                return true;
            default:
                return false;
        }
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
    Line * line = linePanel.data;
    Side * side = &line->sides[line->panelBackSelected];

    int i;
    for ( i = 0; i < map.lines->count; i++ )
    {
        Line * check = Get(map.lines, i);
        if ( check == line )
            break;
    }

    char title[100] = { 0 };
    snprintf(title, sizeof(title), "Line %d", i);
    SetPanelColor(15, 1);
    PanelPrint(&linePanel, 2, 1, title);

    SetPanelColor(14, 1);

    ItemPrint(&linePanel, LP_LOWER, side->bottom);
    ItemPrint(&linePanel, LP_MIDDLE, side->middle);
    ItemPrint(&linePanel, LP_UPPER, side->top);
}

#pragma mark - RENDER

static void RenderLinePanel(void)
{
    const Line * line = linePanel.data;
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
    SetPanelRenderColor(14);

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
    linePanel.location.x = PANEL_SCREEN_MARGIN;
    linePanel.location.y = PANEL_SCREEN_MARGIN;
    linePanel.items = linePanelItems;
    linePanel.numItems = LP_NUM_ITEMS;
    linePanel.render = RenderLinePanel;
    linePanel.processEvent = ProcessLinePanelEvent;
    linePanel.selection = 1;

    printf("Loading line specials from %s...\n", dspPath);
    LoadSpecials(dspPath);

    specialCategoriesPanel = LoadPanel(PANEL_DATA_DIRECTORY"special_categories.panel");
    specialCategoriesPanel.items = specialCategoryItems;
    specialCategoriesPanel.numItems = numCategories + 1;
//    specialCategoriesPanel.parent = &linePanel;
    specialCategoriesPanel.location.x = linePanel.location.x + linePanel.location.w + 8;
    specialCategoriesPanel.location.y = linePanel.location.y + linePanel.location.h - specialCategoriesPanel.location.h;
    specialCategoriesPanel.render = NULL;
    specialCategoriesPanel.processEvent = ProcessSpecialCategoriesPanelEvent;

    if ( numCategories != NUM_SPECIAL_CATEGORIES )
        printf("!!! warning: numCategories != NUM_SPECIAL_CATEGORIES\n");
}

void FreeLinePanels(void)
{
    FreePanel(&linePanel);
    FreePanel(&specialCategoriesPanel);
    FreePanel(&specialsPanel);
}
