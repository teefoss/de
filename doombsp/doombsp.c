// doombsp.c
#include "doombsp.h"
#include "edit.h"
#include "map.h"

bool draw;

/// Build the currently loaded map and add/replace in editor.pwad.
void DoomBSP(void)
{
    draw = false; // TODO: use a key modifier to show the build process window.

    int index = GetLumpIndexFromName(editor.pwad, map.label);

    // If this map already exists in the WAD, it will be replaced.
    if ( index != -1 )
    {
        // TODO: find and alternative to deleting the WAD, in case building fails!
        RemoveMap(editor.pwad, map.label);
    }

    AddLump(editor.pwad, map.label, map.label, 0);

    NB_LoadMap();
	NB_DrawMap();
	BuildBSP();
	
    printf ("segment cuts: %i\n", cuts);

	SaveDoomMap();
	SaveBlocks();
    WriteDirectory(editor.pwad);

    printf("Node building complete.\n");
}
