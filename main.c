//
//  main.cc
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "wad.h"
#include "map.h"
#include "doomdata.h"
#include "window.h"
#include "mapview.h"

#include <SDL2/SDL.h>
#include <string.h>
#include <stdbool.h>

// de --help, -h
// de <subprogram> (<command>) [arguments]

//    0     1       2                           3                   4
// de wad   --help, -h
// de wad   list    [WAD file](:[lump name])
// de wad   copy    [source WAD]:[lump name]    [destination WAD]
// de wad   swap    [WAD file]                  [lump name 1]       [lump name 2]
// de wad   remove  [WAD file]:[lump name]

// de edit  [WAD file] --iwad [WAD file]

static int     _argc;
static char ** _argv;

int GetArg(const char * string)
{
    size_t len = strlen(string);

    for ( int i = 0; i < _argc; i++ ) {
        if ( strncmp(string, _argv[i], len) == 0 ) {
            return i;
        }
    }

    return -1;
}

char * GetOptionArg(const char * option)
{
    int index = GetArg(option);
    if ( index == -1 || index + 1 >= _argc ) {
        return NULL;
    }

    return _argv[index + 1];
}

void ListWadFile(const char * path)
{
//    WadFile wad;
}

void PrintUsageAndExit(void)
{
    fprintf(stderr,
            "usage:\n"
            "  de edit [WAD path] --map [map name*] --iwad [resource WAD path]\n"
            "  *map name: e[1-4]m[1-9] or map[01-32]");
    exit(EXIT_FAILURE);
}

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

void DoSubprogramWad(int argc, char ** argv)
{
    if ( argc < 2 )
        PrintUsageAndExit();

    char * command = argv[1];

    if ( STRNEQ(command, "list", 4) )
    {
        if ( argc != 3 )
        {
            fprintf(stderr, "usage: de wad list [WAD file](:[lump name])");
            exit(EXIT_FAILURE);
        }

        char * wadPath = argv[2];
        char * lumpName = GetLumpNameFromArg(&wadPath);

        Wad * wad = OpenWad(wadPath);
        if ( wad == NULL )
        {
            fprintf(stderr, "Error: could not open `%s`\n", wadPath);
            exit(EXIT_FAILURE);
        }

        printf("%s\n", wadPath);
        printf("type: %s\n", GetWadType(wad));
        printf("lump count: %d\n", wad->directory->count);

        if ( lumpName )
            printf("lumps named '%s':\n", lumpName);
        else
            printf("lumps:\n");

        for ( int i = 0; i < wad->directory->count; i++ )
        {
            LumpInfo * info = Get(wad->directory, i);
            char name[9] = { 0 };
            strncpy(name, info->name, 8);

            if ( lumpName && !STRNEQ(lumpName, name, 8) )
                continue;

            printf(" %d: %s (%d bytes)\n", i, name, info->size);
        }

        exit(EXIT_SUCCESS);
    }
    else if ( STRNEQ(command, "copy", 4) )
    {
        // de wad   copy (--map) [source WAD]:[lump name] [destination WAD]

        bool copyMap;
        char * sourcePath;
        char * destinationPath;

        if ( STRNEQ(argv[2], "--map", 5) ) {
            copyMap = true;
            sourcePath = argv[3];
            destinationPath = argv[4];
        } else {
            copyMap = false;
            sourcePath = argv[2];
            destinationPath = argv[3];
        }

        char * lumpName = GetLumpNameFromArg(&sourcePath);
        if ( lumpName == NULL ) {
            fprintf(stderr, "Error: expected lump name to copy\n"
                    "usage: de wad copy [source wad file]:[lump name] [destination wad file]\n");
            exit(EXIT_FAILURE);
        }

        Wad * source = OpenWad(sourcePath);
        if ( source == NULL )
        {
            fprintf(stderr, "Error: could not open source WAD '%s'\n", sourcePath);
            exit(EXIT_FAILURE);
        }

        Wad * destination = OpenWad(destinationPath);
        if ( destination == NULL )
        {
            printf("Creating copy destination '%s'\n", destinationPath);

            destination = CreateWad(destinationPath);
            if ( destination == NULL )
            {
                fprintf(stderr, "Error: could not create '%s'\n", destinationPath);
                exit(EXIT_FAILURE);
            }
        }

        if ( copyMap )
        {
//            int lumpIndex = source.getLumpIndex(lumpName);
            int lumpIndex = GetLumpIndex(source, lumpName);

            for ( int i = lumpIndex; i < lumpIndex + ML_COUNT; i++ )
            {
                void * lump = GetLumpWithIndex(source, i);
                u32 size = GetLumpSize(source, i);
                const char * name = GetLumpName(source, i);

                AddLump(destination, name, lump, size);

                free(lump);
            }
        }
        else
        {
            int i = GetLumpIndex(source, lumpName);
            void * lump = GetLumpWithName(source, lumpName);
            AddLump(destination, lumpName, lump, GetLumpSize(source, i));
            free(lump);
        }

        printf(copyMap
               ? "Copied map '%s' lumps from '%s' to '%s'\n"
               : "Copied lump '%s' from '%s' to '%s'\n",
               lumpName,
               sourcePath,
               destinationPath);

        exit(EXIT_SUCCESS);
    } else if ( STRNEQ(command, "remove", 6) ) {
        char * lumpName = GetLumpNameFromArg(&argv[2]);

        Wad * wad = OpenWad(argv[2]);
        if ( wad == NULL ) {
            fprintf(stderr, "Error: could not open '%s'\n", argv[2]);
            exit(EXIT_FAILURE);
        }

        RemoveLump(wad, lumpName);
        printf("Removed lump '%s' from '%s'.\n", lumpName, argv[2]);

        FreeWad(wad);

        return exit(EXIT_SUCCESS);
    }
}

int main(int argc, char ** argv)
{
    printf("de\n"
           "[d]oom [e]ditor\n"
           "v. 0.1 Copyright (C) 2023 Thomas Foster\n\n");

    argc--;
    argv++;
    _argc = argc;
    _argv = argv;

    if ( argc < 2 )
        PrintUsageAndExit();

    Wad * editWad = NULL;
    Wad * resourceWad = NULL;

    char * subprogram = argv[0];

    // Parse arguments for subprograms.
    if ( STRNEQ(subprogram, "wad", 3) )
    {
        DoSubprogramWad(argc, argv);
    }
    else if ( STRNEQ(subprogram, "edit", 4 ) )
    {
        // de edit [WAD file] --map [map label] --iwad [WAD file]

        if ( argc != 6 )
            PrintUsageAndExit();

        char * fileToEdit = argv[1];

        editWad = OpenWad(fileToEdit);
        if ( editWad == NULL )
        {
            printf("Creating %s...\n", fileToEdit);
            editWad = CreateWad(fileToEdit);
            if ( editWad == NULL ) {
                fprintf(stderr, "Error: could not create '%s'\n", fileToEdit);
                return EXIT_FAILURE;
            }
        }

        char * mapLabel = GetOptionArg("--map");
        if ( mapLabel == NULL ) {
            fprintf(stderr, "Error: missing --map option");
            PrintUsageAndExit();
        }

        LoadMap(editWad, mapLabel);

        char * iwadName = GetOptionArg("--iwad");
        if ( iwadName == NULL ) {
            fprintf(stderr, "Error: missing --iwad option");
            PrintUsageAndExit();
        }

        resourceWad = OpenWad(iwadName);
        if ( resourceWad == NULL ) {
            fprintf(stderr, "Error: could not load resource WAD '%s'\n", iwadName);
            PrintUsageAndExit();
        }
    }

    InitWindow();
    InitMapView();

    const u8 * keys = SDL_GetKeyboardState(NULL);

    const float dt = 1.0f / GetRefreshRate();

    bool run = true;
    while ( run )
    {
        SDL_Event event;
        while ( SDL_PollEvent(&event) )
        {
            switch ( event.type )
            {
                case SDL_QUIT:
                    run = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch ( event.window.event )
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            visibleRect.w = event.window.data1;
                            visibleRect.h = event.window.data2;
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_KEYDOWN:
                    switch ( event.key.keysym.sym )
                    {
                        case SDLK_UP:
//                            visibleRect.y -= 16;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }

        if ( keys[SDL_SCANCODE_UP]    ) visibleRect.y -= 256.0f * dt;
        if ( keys[SDL_SCANCODE_DOWN]  ) visibleRect.y += 256.0f * dt;
        if ( keys[SDL_SCANCODE_LEFT]  ) visibleRect.x -= 256.0f * dt;
        if ( keys[SDL_SCANCODE_RIGHT] ) visibleRect.x += 256.0f * dt;

        SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
        SDL_RenderClear(renderer);

        DrawMap();

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
