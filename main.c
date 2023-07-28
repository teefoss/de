//
//  main.c
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "wad.h"
#include "m_map.h"
#include "doomdata.h"
#include "common.h"
//#include "e_map_view.h"
#include "args.h"
#include "e_editor.h"
#include "p_line_panel.h"
#include "p_thing_panel.h"
//#include "g_patch.h"
//#include "p_progress_panel.h"
//#include "p_texture_panel.h"
#include "e_defaults.h"
//#include "p_sector_panel.h"
//#include "g_flat.h"

#include <SDL2/SDL.h>
#include <string.h>
#include <stdbool.h>

// Bugs List

// TODO List
// TODO: 'New Texture' button in texture palette
// TODO: 'Edit Texture' button in texture palette
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
    }
    else
    {
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

int RunEditor(const char * wadPath, char * mapName)
{
    printf("[d]oom [e]ditor\n"
           "v. 0.1 Copyright (C) 2023 Thomas Foster\n\n");

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

    if ( !LoadMap(editor.pwad, mapName) )
        CreateMap(mapName);
    InitWindow(800, 800); // TODO: save user's favorite window size and position.

    Capitalize(mapName);
    SDL_SetWindowTitle(window, mapName);


    DetermineGame(iwadPath);

    printf("Editing %s in '%s'\n\n", mapName, wadPath);

    InitEditor();
    EditorLoop();
    CleanupEditor();

    return EXIT_SUCCESS;
}

void ParseListCommand(const char * wadPath)
{
    if ( GetArg2("--list", "-ls") != -1 )
    {
        Wad * wad = OpenWad(wadPath);
        ListDirectory(wad);
        FreeWad(wad);
        exit(0);
    }
}

void ParseCopyCommand(const char * wadPath)
{
    char * copySource = GetOptionArg2("--copy", "-cp");

    if ( copySource )
    {
        Wad * destinationWAD = OpenOrCreateWad(wadPath);

        char * lumpName = GetLumpNameFromArg(&copySource);

        Wad * sourceWAD = OpenWad(copySource);
        if ( sourceWAD == NULL )
        {
            exit(EXIT_FAILURE);
        }

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
}

int main(int argc, char ** argv)
{
    if ( argc == 1 )
    {
        printf("Bad arguments! Here's some help:\n\n");
        // TODO: print help!
        exit(EXIT_FAILURE);
    }

    InitArgs(--argc, ++argv);

    if ( GetArg2("--help", "-h") != -1 )
    {
        // TODO: write help text!
        printf("Help coming soon\n");
        return EXIT_SUCCESS;
    }

    char * wadPath = argv[0];

    ParseListCommand(wadPath);


    char * lumpToRemove = GetOptionArg2("--remove-number", "-rm-num");
    if ( lumpToRemove )
    {
        Wad * wad = OpenWad(wadPath);
        RemoveLumpNumber(wad, atoi(lumpToRemove));
        SaveWAD(wad);
        FreeWad(wad);
        return EXIT_SUCCESS;
    }

    lumpToRemove = GetOptionArg2("--remove", "-rm");
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

    ParseCopyCommand(wadPath);

    char * mapName = GetOptionArg2("--edit", "-e");
    if ( mapName )
        return RunEditor(wadPath, mapName);

    return EXIT_SUCCESS;
}
