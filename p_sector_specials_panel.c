//
//  p_sector_specials_panel.c
//  de
//
//  Created by Thomas Foster on 7/22/23.
//

#include "p_sector_specials_panel.h"
#include "p_panel.h"
#include "p_stack.h"
#include "p_sector_panel.h"
#include "e_editor.h"

Panel sectorSpecialsPanel;

extern SectorDef baseSectordef;
extern Panel sectorPanel;

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


const char * GetSpecialName(int id)
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
    if ( DidClickOnItem(event, &sectorSpecialsPanel) )
    {
        int id = sectorSpecials[sectorSpecialsPanel.selection].id;
        baseSectordef.special = id;
        SectorPanelApplyChange();
        CloseTopPanel();
        return true;
    }

    return false;
}


void LoadSectorSpecialsPanel(void)
{
    if ( editor.game == GAME_DOOM1 )
    {
        LoadPanelConsole(&sectorSpecialsPanel,
                         PANEL_DATA_DIRECTORY "sector_specials.panel");
        sectorSpecialsPanel.numItems = SDL_arraysize(specialItems);
        sectorSpecialsPanel.items = specialItems;
    }
    else
    {
        LoadPanelConsole(&sectorSpecialsPanel,
                  PANEL_DATA_DIRECTORY "sector_specials2.panel");
        sectorSpecialsPanel.numItems = SDL_arraysize(specialItems2);
        sectorSpecialsPanel.items = specialItems2;
    }

    for ( int i = 0; i < sectorSpecialsPanel.numItems; i++ )
    {
        sectorSpecialsPanel.items[i].x = 2;
        sectorSpecialsPanel.items[i].width = 23;
    }

    sectorSpecialsPanel.processEvent = ProcessSpecialPanelEvent;
}
