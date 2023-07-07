//
//  main.c
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "wad.h"
#include "map.h"
#include "doomdata.h"
#include "common.h"
#include "mapview.h"
#include "args.h"
#include "edit.h"
#include "line_panel.h"
#include "thing_panel.h"
#include "patch.h"
#include "progress_panel.h"
#include "texture_panel.h"
#include "defaults.h"
#include "sector_panel.h"
#include "flat.h"

#include <SDL2/SDL.h>
#include <string.h>
#include <stdbool.h>

// Bugs List
// FIXME: closing texture pal should clear filters.
// FIXME: texture palette items location messed up.

// TODO List
// TODO: line.c
// TODO: 'Remove Texture' button in texture palette
// TODO: 'New Texture' button in texture palette
// TODO: 'Edit Texture' button in texture palette
// TODO: Display size in texture palette
// TODO: Thing palette: "all", with filter?
// TODO: LoadPanel -> LoadPanelConsole, move data init to global struct init.
// TODO: selection hover in texture palette
// TODO: player starts sprites: use correct palette colors
// TODO: texture palette wide enough for 4 x 64 width textures, and resize vert.
// TODO: change image well to solid background
// TODO: refactor scroll bar: add 'scroll amount' and 'max scroll'
// TODO: find an replace toupper loops with SDL_strupr
// TODO: stop using Error() in node builder

// Nice-to-have TODO list
// TODO: animated sprites?
// TODO: spectre sprite fuzz effect

// de --help, -h
// de <subprogram> (<command>) [arguments]

//    0     1       2                           3                   4
// de wad   --help, -h
// de wad   list    [WAD file](:[lump name])
// de wad   copy    [source WAD]:[lump name]    [destination WAD]
// de wad   swap    [WAD file]                  [lump name 1]       [lump name 2]
// de wad   remove  [WAD file]:[lump name]

// de edit  [WAD file] --iwad [WAD file]


// TODO: change to:
// de [WAD path] (options)
//
// de [WAD path] --edit e1m1 --iwad [WAD path]
// de [WAD path] --list-maps
// de [WAD path] --list-all
// ... etc
// de [WAD path] --copy/-cp [source WAD]:[lump name]
// de [WAD path] --remove/-rm [lump name]
// de [WAD path] --set-type [iwad or pwad]




//void ListWadFile(const char * path)
//{
////    WadFile wad;
//}

// Get the lump name from an arg of the format 'file.wad:LUMPNAME'
// The lump name is returned and also stripped from `arg`.
char * GetLumpNameFromArg(char ** arg)
{
    char * lumpName = strchr(*arg, ':');
    if ( lumpName )
    {
        *lumpName = '\0';
        lumpName++;
    }

    return lumpName;
}

/// Set `editor.game` according to the name of the IWAD.
void DetermineGame(char * iwadName)
{
    char * gameParam = GetOptionArg2("--game", "-g");
    Capitalize(gameParam);

    if ( gameParam != NULL )
    {
        printf("Game: %s\n", gameParam);

        if ( strcmp(gameParam, "DOOM") == 0 )
            editor.game = GAME_DOOM1;
        else if ( strcmp(gameParam, "DOOMSE") )
            editor.game = GAME_DOOM1SE;
        else if ( strcmp(gameParam, "DOOM2") )
            editor.game = GAME_DOOM2;
        else
        {
            printf("Error: bad game argument. Should be"
                    "'doom', 'doomse', or 'doom2'\n");
            exit(EXIT_FAILURE);
        }
    } else {
        // -g or --game was not specified, determine which game from the IWAD name.
        Capitalize(iwadName); // TODO: write CaseCompare!

        if ( strcmp(iwadName, "DOOM1.WAD") == 0 )
        {
            editor.game = GAME_DOOM1;
            printf("Game: Doom\n");
        }
        else if ( strcmp(iwadName, "DOOM.WAD") == 0 )
        {
            editor.game = GAME_DOOM1SE;
            printf("Game: The Ultimate Doom SE\n");
        }
        else if ( strcmp(iwadName, "DOOM2.WAD") == 0 )
        {
            editor.game = GAME_DOOM2;
            printf("Game: Doom 2\n");
        }
        else
        {
            printf("Unable to determine game, using Doom 2.\n"
                   "If this is not what you want, please specify which game"
                   "with --game (options 'doom', 'doomse', 'doom2')\n");
            editor.game = GAME_DOOM2;
        }
    }

}

int DoSubprogramEdit(int argc, char ** argv)
{
    // de edit [WAD file] --map [map label]
    // optional args:
    //      --iwad (if not present, try to read from project config)
    //      -g, --game (if not present, try to read from project config,
    //                  or if project config not present, try to guess)

    LoadDefaults("de.config");

    // At the very least, we should have 'edit [wad] --map [name]]'
    if ( argc < 4 )
    {
        printf("Error: bad arguments!\n");
        return EXIT_FAILURE;
    }

    // Open or create the specified PWAD.

    // TODO: if it's an IWAD and the name is doom.wad, etc. warn and quit.
    // This is a failsafe to prevent editing the original wads.
    // (User can rename the wad if they really want to do this.)

    char * fileToEdit = argv[1];

    editor.pwad = OpenWad(fileToEdit);
    if ( editor.pwad == NULL )
    {
        printf("Creating %s...\n", fileToEdit);
        editor.pwad = CreateWad(fileToEdit);
        if ( editor.pwad == NULL ) {
            fprintf(stderr, "Error: could not create '%s'\n", fileToEdit);
            return EXIT_FAILURE;
        }
    }

    // Get the map label (required).

    char * mapLabel = GetOptionArg("--map");
    if ( mapLabel == NULL ) {
        fprintf(stderr, "Error: missing --map option");
        goto print_usage;
    }

    // Get the specified iwad name (optional)

    char * iwadName = GetOptionArg("--iwad");
    if ( iwadName == NULL ) {
        // TODO: make optional, read project file when not present.
        fprintf(stderr, "Error: missing --iwad option");
        goto print_usage;
    }

    editor.iwad = OpenWad(iwadName);
    if ( editor.iwad == NULL ) {
        fprintf(stderr, "Error: could not load IWAD '%s'\n", iwadName);
        return EXIT_FAILURE;
    }

    LoadMap(editor.pwad, mapLabel);
    InitWindow(800, 800); // TODO: save user's favorite window size and position.

    DetermineGame(iwadName);

    InitEditor();
    EditorLoop();

    CleanupEditor();
    return EXIT_SUCCESS;

print_usage:
    printf("Usage: de edit [WAD path] --map [name] (--iwad [path])");
    return EXIT_FAILURE;
}

void CreateProject(void)
{
    FILE * project = fopen("project.de", "w");
    if ( project == NULL )
    {
        printf("Error: could not create project file!\n");
        return;
    }

    fprintf(project, "de project file version 1\n\n");
    fprintf(project, "iwad: doom2.wad\n");
    fprintf(project, "pwad: mywad.wad");
    fprintf(project, "node_builder: \n");
    fprintf(project, "run: \n");
    fprintf(project, "maps: \n");

    fclose(project);

    printf("Created a new project.\n"
           "Please set up the project parameters in 'project.de'\n");
}

int RunEditor(const char * wadPath, const char * mapName)
{
    LoadDefaults("de.config");

    editor.pwad = OpenWad(wadPath);
    if ( editor.pwad == NULL )
    {
        printf("Creating %s...\n", wadPath);
        editor.pwad = CreateWad(wadPath);
        if ( editor.pwad == NULL )
            return EXIT_FAILURE;
    }

    char * iwadPath = GetOptionArg("--iwad");
    if ( iwadPath == NULL )
    {
        // TODO: make optional, read project file when not present.
        fprintf(stderr, "Error: missing --iwad option\n");
        return EXIT_FAILURE;
    }

    editor.iwad = OpenWad(iwadPath);
    if ( editor.iwad == NULL )
    {
        fprintf(stderr, "Error: could not load IWAD '%s'\n", iwadPath);
        return EXIT_FAILURE;
    }
    printf("Using IWAD '%s'.\n", iwadPath);

    LoadMap(editor.pwad, mapName);

    InitWindow(800, 800); // TODO: save user's favorite window size and position.

    DetermineGame(iwadPath);

    printf("Editing %s in '%s'\n\n", mapName, wadPath);

    InitEditor();
    EditorLoop();
    CleanupEditor();

    return EXIT_SUCCESS;
}

void CopyLump(Wad * destination, const Wad * source, int lumpIndex)
{
    Lump * lump = GetLump(source, lumpIndex);
    AddLump(destination, lump->name, lump->data, lump->size);
    SaveWAD(destination);
}

int main(int argc, char ** argv)
{
    printf("[d]oom [e]ditor\n"
           "v. 0.1 Copyright (C) 2023 Thomas Foster\n\n");

    if ( argc == 1 )
        goto error;

    InitArgs(--argc, ++argv);

    if ( GetArg2("--help", "-h") != -1 )
    {
        // TODO: write help text!
        printf("Help coming soon\n");
        return EXIT_SUCCESS;
    }

    char * wadPath = argv[0];

    if ( GetArg2("--list", "-ls") != -1 )
    {
        Wad * wad = OpenWad(wadPath);
        ListDirectory(wad);
        FreeWad(wad);
    }

    char * lumpToRemove = GetOptionArg2("--remove-number", "-rm-num");
    if ( lumpToRemove )
    {
        Wad * wad = OpenWad(wadPath);
        RemoveLumpNumber(wad, atoi(lumpToRemove));
        SaveWAD(wad);
        FreeWad(wad);
    }

    lumpToRemove = GetOptionArg("--remove");
    if ( lumpToRemove )
    {
        Wad * wad = OpenWad(wadPath);

        if ( GetArg("--map") != -1 )
            RemoveMap(wad, lumpToRemove);
        else
            RemoveLumpNamed(wad, lumpToRemove);

        SaveWAD(wad);
        FreeWad(wad);
    }

    char * copySource = GetOptionArg2("--copy", "-cp");
    if ( copySource )
    {
        Wad * destinationWAD = OpenOrCreateWad(wadPath);

        char * lumpName = GetLumpNameFromArg(&copySource);
        Wad * sourceWAD = OpenWad(copySource);
        if ( sourceWAD == NULL )
            return EXIT_FAILURE;

        int index = GetIndexOfLumpNamed(sourceWAD, lumpName);

        if ( GetArg2("--map", "-m") != -1 )
        {
            for ( int i = index; i < index + ML_COUNT; i++ )
                CopyLump(destinationWAD, sourceWAD, i);

            printf("Copied '%s' map lumps to '%s'\n", lumpName, wadPath);
        }
        else
        {
            CopyLump(destinationWAD, sourceWAD, index);
            printf("Copied lump '%s' to '%s'\n", lumpName, wadPath);
        }

        SaveWAD(destinationWAD);
        FreeWad(sourceWAD);
        FreeWad(destinationWAD);
    }

    char * mapName = GetOptionArg2("--edit", "-e");
    if ( mapName )
        return RunEditor(wadPath, mapName);

    return EXIT_SUCCESS;
error:
    printf("Error: bad arguments! Use 'de --help'\n");
    return EXIT_FAILURE;
}
