// doombsp.c
#include "doombsp.h"
#include "edit.h"
#include "map.h"

bool draw;

/// Build the currently loaded map and add/replace in editor.pwad.
void DoomBSP(void)
{
    draw = false; // TODO: use a key modifier to show the build process window.
    bool replace;

    int index = GetIndexOfLumpNamed(editor.pwad, map.label);
    if ( -index == -1 )
    {
        editor.pwad->position = editor.pwad->lumps->count;
        replace = false;
    }
    else
    {
        editor.pwad->position = index;
        replace = true;
    }

    AddLump(editor.pwad, map.label, map.label, 0);

    NB_LoadMap();
	NB_DrawMap();
	BuildBSP();
	
    printf ("segment cuts: %i\n", cuts);

	SaveDoomMap();
	SaveBlocks();

    if ( replace )
    {
        for ( int i = 0; i < ML_COUNT; i++ )
            RemoveLumpNumber(editor.pwad, editor.pwad->position);
    }

    SaveWAD(editor.pwad);

    printf("Node building complete.\n");
//    ListDirectory(editor.pwad);
}
