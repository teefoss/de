// doombsp.c
#include "doombsp.h"
#include "edit.h"
#include "map.h"


boolean		draw;

/// Build the currently loaded map and add to editor.pwad.
void DoomBSP(void)
{
    draw = false;

    int index = GetLumpIndexFromName(editor.pwad, map.label);

    // If this map already exists in the WAD, it will be replaced.
    if ( index != -1 )
    {
        // TODO: find and alternative to deleting the WAD, in case building fails!
        RemoveMap(editor.pwad, map.label);
    }

    AddLump(editor.pwad, map.label, map.label, 0);

    NodeBuilderLoadMap();
	NodeBuilderDrawMap();

	BuildBSP();
	
    printf ("segment cuts: %i\n", cuts);

	SaveDoomMap();
	SaveBlocks();

    WriteDirectory(editor.pwad);

    printf("Post-building directory:\n");
    ListDirectory(editor.pwad);

    printf("Node building complete.\n");
}
