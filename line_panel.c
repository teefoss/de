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

#include "doomdata.h"

#define NUM_SPECIAL_CATEGORIES 7
#define NUM_SPECIAL_ROWS 14

enum
{
    LP_FRONT = 1,
    LP_BACK,

    LP_LOWER,
    LP_MIDDLE,
    LP_UPPER,
    LP_LOWER_NAME,
    LP_MIDDLE_NAME,
    LP_UPPER_NAME,
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
    { 20, 3, 5, -1, LP_MIDDLE, -1, LP_BACK }, // front
    { 34, 3, 4, -1, LP_UPPER, LP_FRONT, -1 }, // back

    { 6, 4, 5, LP_FRONT, LP_LOWER_NAME, -1, LP_MIDDLE },
    { 20, 4, 6, LP_FRONT, LP_MIDDLE_NAME, LP_LOWER, LP_UPPER },
    { 34, 4, 5, LP_BACK, LP_UPPER_NAME, LP_MIDDLE, -1 },

    { 7, 12, 8, LP_LOWER, LP_OFFSET_X, -1, LP_MIDDLE_NAME },
    { 21, 12, 8, LP_MIDDLE, LP_OFFSET_X, LP_LOWER_NAME, LP_UPPER_NAME },
    { 35, 12, 8, LP_UPPER, LP_OFFSET_Y, LP_MIDDLE_NAME, -1 },

    { 14, 13, 4, LP_LOWER_NAME, LP_BLOCKS_ALL, -1, LP_OFFSET_Y }, // offset x
    { 35, 13, 4, LP_UPPER_NAME, LP_TWO_SIDED, LP_OFFSET_X, -1 }, // offset y

    { 8, 16, 16, LP_OFFSET_X, 0, -1, LP_TWO_SIDED }, // LP_BLOCKS_ALL,
    { 8, 17, 16, 0, 0, -1, LP_ALWAYS_ON_MAP }, // LP_BLOCKS_MONSTERS,
    { 8, 18, 16, 0, LP_UPPER_UNPEGGED, -1, LP_NOT_ON_MAP }, // LP_BLOCKS_SOUND,
    { 29, 16, 16, LP_OFFSET_Y, LP_ALWAYS_ON_MAP, LP_BLOCKS_ALL, -1 }, //     LP_TWO_SIDED,
    { 8, 19, 16, LP_BLOCKS_SOUND, 0, -1, LP_SECRET }, // LP_UPPER_UNPEGGED,
    { 8, 20, 16, 0, LP_SPECIAL, -1, LP_SECRET }, // LP_LOWER_UNPEGGED,
    { 29, 17, 16, LP_TWO_SIDED, 0, LP_BLOCKS_MONSTERS, -1 }, // LP_ALWAYS_ON_MAP,
    { 29, 18, 16, 0, 0, LP_BLOCKS_SOUND, -1 }, // LP_NOT_ON_MAP,
    { 29, 19, 16, 0, LP_SPECIAL, LP_UPPER_UNPEGGED, -1 }, // LP_SECRET,

    { 4, 23, 30, LP_LOWER_UNPEGGED, 0, -1, -1 }, // special
    { 9, 24, 4, 0, -1, -1, LP_SUGGEST_TAG }, // tag
    { 25, 24, 9, LP_SPECIAL, -1, LP_TAG, -1 }, // suggest
};

static PanelItem specialCategoryItems[] =
{
    { 6, 3, 13, -1, 0, -1, -1 },
    { 6, 4, 13, 0, 0, -1, -1 },
    { 6, 5, 13, 0, 0, -1, -1 },
    { 6, 6, 13, 0, 0, -1, -1 },
    { 6, 7, 13, 0, 0, -1, -1 },
    { 6, 8, 13, 0, 0, -1, -1 },
    { 6, 9, 13, 0, 0, -1, -1 },
    { 6, 10, 13, 0, -1, -1, -1 }
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
} SpecialCategory;

SpecialCategory categories[NUM_SPECIAL_CATEGORIES];
int numCategories = 0;
int selectedCategory; // Index into `categories`
int selectedSpecial;
int firstSpecial; // Index of topmost visible special in list.

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

    long maxShortNameLen = 0;

    for ( int i = 0; i < numSpecials; i++ )
    {
        int id;
        fscanf(file, "%d:", &id);
        specials[i].id = id;

        fscanf(file, "%s\n", specials[i].name);

        // Extract the category name.
        char category[80] = { 0 };
        strncpy(category, specials[i].name, 80);
        long firstSpaceIndex = strchr(category, '_') - category;
        specials[i].shortName = &specials[i].name[firstSpaceIndex + 1];

        if ( strlen(specials[i].shortName) > maxShortNameLen )
            maxShortNameLen = strlen(specials[i].shortName);

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
        printf("Cateogry %d: %s; start: %d; count: %d\n",
               i,
               categories[i].name,
               categories[i].startIndex,
               categories[i].count);

    printf("max short name length: %ld\n", maxShortNameLen);
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
                UpdatePanel(&specialsPanel,
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
            SDL_itoa(line->tag, linePanel.text, 10);
            StartTextEditing(&linePanel, LP_TAG, &line->tag);
            break;

        case LP_OFFSET_X:
            SDL_itoa(side->offsetX, linePanel.text, 10);
            StartTextEditing(&linePanel, LP_OFFSET_X, &side->offsetX);
            break;

        case LP_OFFSET_Y:
            SDL_itoa(side->offsetY, linePanel.text, 10);
            StartTextEditing(&linePanel, LP_OFFSET_Y, &side->offsetY);
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
                    if ( linePanel.mouseHover != -1 )
                    {
                        linePanel.selection = linePanel.mouseHover;
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
    openPanels[++topPanel] = &specialsPanel;
    selectedCategory = specialCategoriesPanel.selection - 1;
    firstSpecial = 0;
    specialsPanel.numItems = MIN(categories[selectedCategory].count,
                                 NUM_SPECIAL_ROWS);
    UpdateVisibleSpecials();
}

bool IsMouseActionEvent(const SDL_Event * event)
{
    return event->type == SDL_MOUSEBUTTONDOWN
        && event->button.button == SDL_BUTTON_LEFT;
}

bool IsActionEvent(const SDL_Event * event)
{
    bool isKeyAction
        =  event->type == SDL_KEYDOWN
        && event->key.keysym.sym == SDLK_RETURN;

    return isKeyAction || IsMouseActionEvent(event);
}

static bool ProcessSpecialCategoriesPanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event) )
    {
        if ( IsMouseActionEvent(event) && linePanel.mouseHover != -1 )
            specialCategoriesPanel.selection = specialCategoriesPanel.mouseHover;

        if ( specialCategoriesPanel.selection == 0 )
        {
            ((Line *)linePanel.data)->special = 0;
            topPanel--;
        }
        else
            OpenSpecialsPanel();

        return true;
    }

    return false;
}

bool ProcessSpecialPanelEvent(const SDL_Event * event)
{
    if ( IsActionEvent(event) )
    {
        if ( IsMouseActionEvent(event) && linePanel.mouseHover != -1 )
            specialsPanel.selection = specialsPanel.mouseHover;


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

    ItemPrint(&linePanel, LP_LOWER_NAME, side->bottom);
    ItemPrint(&linePanel, LP_MIDDLE_NAME, side->middle);
    ItemPrint(&linePanel, LP_UPPER_NAME, side->top);
}

#pragma mark - RENDER

static void RenderLinePanel(void)
{
    const Line * line = linePanel.data;
    const Side * side = &line->sides[line->panelBackSelected];

    int windowWidth;
    int windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    linePanel.location.x = windowWidth - linePanel.location.w - FONT_WIDTH * 2;
    linePanel.location.y = FONT_HEIGHT;

    RenderPanelTexture(&linePanel);

    // Render Properties

    SDL_RenderSetViewport(renderer, &linePanel.location);

    PanelItem * items = linePanelItems;

    if ( !linePanel.isTextEditing )
        RenderPanelSelection(&linePanel);

    SDL_Rect textureRect = { .w = 12 * FONT_WIDTH, .h = 6 * FONT_HEIGHT };

    textureRect.x = (items[LP_LOWER].x - 1) * FONT_WIDTH;
    textureRect.y = (items[LP_LOWER].y + 1) * FONT_HEIGHT;
    if ( side->bottom[0] != '-')
        RenderTextureInRect(side->bottom, &textureRect);

    textureRect.x = (items[LP_MIDDLE].x - 1) * FONT_WIDTH;
    if ( side->middle[0] != '-')
        RenderTextureInRect(side->middle, &textureRect);

    textureRect.x = (items[LP_UPPER].x - 1) * FONT_WIDTH;
    if ( side->top[0] != '-')
        RenderTextureInRect(side->top, &textureRect);



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

    if ( !linePanel.isTextEditing || (linePanel.isTextEditing && linePanel.textItem != LP_OFFSET_X) )
    {
        PANEL_PRINT(items[LP_OFFSET_X].x, items[LP_OFFSET_X].y, "%d", side->offsetX);
    }

    if ( !linePanel.isTextEditing || (linePanel.isTextEditing && linePanel.textItem != LP_OFFSET_Y) )
    {
        PANEL_PRINT(items[LP_OFFSET_Y].x, items[LP_OFFSET_Y].y, "%d", side->offsetY);
    }

    int x = items[LP_SPECIAL].x;
    int y = items[LP_SPECIAL].y;

    if ( line->special == 0 )
    {
        PANEL_PRINT(x, y, "(none)");
    }
    else
    {
        LineSpecial * special = FindSpecial(line->special);

        if ( special == NULL )
            PANEL_PRINT(x, y, "Bad Special! (%d)", line->special);
        else
            PANEL_PRINT(x, y, "%s", special->name);
    }

    if ( linePanel.isTextEditing )
        RenderPanelTextInput(&linePanel);

    if ( !linePanel.isTextEditing || (linePanel.isTextEditing && linePanel.textItem != LP_TAG) )
    {
        PANEL_PRINT(items[LP_TAG].x, items[LP_TAG].y, "%d", line->tag);
    }

    SDL_RenderSetViewport(renderer, NULL);
}

static void RenderSpecialCategoriesPanel(void)
{
    RenderPanelTexture(&specialCategoriesPanel);
    RenderPanelSelection(&specialCategoriesPanel);
}

static void RenderSpecialsPanel(void)
{
    RenderPanelTexture(&specialsPanel);

    const SpecialCategory * category = &categories[selectedCategory];

    SDL_Rect panelLocation = PanelRenderLocation(&specialsPanel);
    SDL_RenderSetViewport(renderer, &panelLocation);

    // Print Panel Title

    SetPanelColor(1);
    PrintString(3 * FONT_WIDTH,
                1 * FONT_HEIGHT,
                "%s Specials", category->name);

    RenderPanelSelection(&specialsPanel);

    SDL_RenderSetViewport(renderer, NULL);
}

#pragma mark -

void LoadLinePanels(const char * dspPath)
{
    linePanel = LoadPanel(PANEL_DIRECTORY"newline.panel");
    linePanel.items = linePanelItems;
    linePanel.numItems = LP_NUM_ITEMS;
    linePanel.render = RenderLinePanel;
    linePanel.processEvent = ProcessLinePanelEvent;
    linePanel.selection = 1;

    printf("Loading line specials from %s...\n", dspPath);
    LoadSpecials(dspPath);

    specialCategoriesPanel = LoadPanel(PANEL_DIRECTORY"special_categories.panel");
    specialCategoriesPanel.items = specialCategoryItems;
    specialCategoriesPanel.numItems = numCategories + 1;
    specialCategoriesPanel.parent = &linePanel;
    specialCategoriesPanel.location.x = linePanelItems[LP_SPECIAL].x * FONT_WIDTH;
    specialCategoriesPanel.location.y = (linePanelItems[LP_SPECIAL].y - 3) * FONT_HEIGHT;
    specialCategoriesPanel.render = RenderSpecialCategoriesPanel;
    specialCategoriesPanel.processEvent = ProcessSpecialCategoriesPanelEvent;

    if ( numCategories != NUM_SPECIAL_CATEGORIES )
        printf("!!! warning: numCategories != NUM_SPECIAL_CATEGORIES\n");

    specialsPanel = LoadPanel(PANEL_DIRECTORY"specials.panel");
    specialsPanel.parent = &linePanel;
    specialsPanel.location.x = specialCategoriesPanel.location.x;
    specialsPanel.location.y = specialCategoriesPanel.location.y;
    specialsPanel.render = RenderSpecialsPanel;
    specialsPanel.processEvent = ProcessSpecialPanelEvent;
    specialsPanel.items = specialItems;
    specialsPanel.numItems = NUM_SPECIAL_ROWS;

    for ( int i = 0; i < NUM_SPECIAL_ROWS; i++ )
    {
        specialItems[i].x = 4;
        specialItems[i].y = i + 3;
        specialItems[i].width = 26;
        specialItems[i].left = specialItems[i].right = -1;
    }

    specialItems[0].up = -1;
    specialItems[NUM_SPECIAL_ROWS - 1].down = -1;
}
