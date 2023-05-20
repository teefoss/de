//
//  main.cc
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "WadFile.h"
#include "Map.h"
#include "DoomData.h"

#include <SDL2/SDL.h>
#include <string.h>

// de --help, -h
// de <subprogram> (<command>) [arguments]

//    0     1       2                           3                   4
// de wad   --help, -h
// de wad   list    [WAD file](:[lump name])
// de wad   copy    [source WAD]:[lump name]    [destination WAD]
// de wad   swap    [WAD file]                  [lump name 1]       [lump name 2]
// de wad   remove  [WAD file]:[lump name]

// de edit [WAD file] --iwad [WAD file]

void ListWadFile(const char * path)
{
    WadFile wad;
}

void PrintUsageAndExit()
{
    fprintf(stderr,
            "usage:\n"
            "  de edit [WAD path] -map [map name*] -iwad [resource WAD path]\n"
            "  de list [WAD path]\n\n"
            "  *map name: e[1-4]m[1-9] or map[01-32]");
    exit(EXIT_FAILURE);
}

// Get the lump name from an arg of the format 'file.wad:LUMPNAME'
// The lump name is returned and also stripped from `arg`.
char * GetLumpName(char ** arg)
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
        char * lumpName = GetLumpName(&wadPath);

        WadFile wad;
        if ( !wad.open(wadPath) )
        {
            fprintf(stderr, "Error: could not open `%s`\n", wadPath);
            exit(EXIT_FAILURE);
        }

        printf("%s\n", wadPath);
        printf("type: %s\n", wad.getType());
        printf("lump count: %d\n", wad.directory.count());

        if ( lumpName )
            printf("lumps named '%s':\n", lumpName);
        else
            printf("lumps:\n");

        for ( int i = 0; i < wad.directory.count(); i++ )
        {
            char name[9] = { 0 };
            strncpy(name, wad.directory[i].name, 8);

            if ( lumpName && !STRNEQ(lumpName, name, 8) )
                continue;

            printf(" %d: %s (%d bytes)\n", i, name, wad.directory[i].size);
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

        char * lumpName = GetLumpName(&sourcePath);
        if ( lumpName == NULL ) {
            fprintf(stderr, "Error: expected lump name to copy\n"
                    "usage: de wad copy [source wad file]:[lump name] [destination wad file]\n");
            exit(EXIT_FAILURE);
        }

        WadFile source;
        if ( !source.open(sourcePath) )
        {
            fprintf(stderr, "Error: could not open source WAD '%s'\n", sourcePath);
            exit(EXIT_FAILURE);
        }

        WadFile destination;
        if ( !destination.open(destinationPath) )
        {
            printf("Creating copy destination '%s'\n", destinationPath);

            if ( !destination.create(destinationPath) )
            {
                fprintf(stderr, "Error: could not create '%s'\n", destinationPath);
                exit(EXIT_FAILURE);
            }
        }

        if ( copyMap )
        {
            int lumpIndex = source.getLumpIndex(lumpName);

            for ( int i = lumpIndex; i < lumpIndex + ML_COUNT; i++ )
            {
                void * lump = source.getLump(i);
                u32 size = source.getLumpSize(i);
                const char * name = source.getLumpName(i);

                destination.addLump(name, lump, size);
                free(lump);
            }
        }
        else
        {
            void * lump = source.getLump(lumpName);
            destination.addLump(lumpName, lump, source.getLumpSize(lumpName));
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
        char * lumpName = GetLumpName(&argv[2]);

        WadFile wad;
        if ( !wad.open(argv[2]) ) {
            fprintf(stderr, "Error: could not open '%s'\n", argv[2]);
            exit(EXIT_FAILURE);
        }

        wad.removeLump(lumpName);
        printf("Removed lump '%s' from '%s'.\n", lumpName, argv[2]);

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

    if ( argc < 2 )
        PrintUsageAndExit();

    WadFile editWad;
    WadFile resourceWad;
    Map map;

    char * subprogram = argv[0];

    // Parse arguments for subprograms.
    if ( STRNEQ(subprogram, "wad", 3) )
    {
        DoSubprogramWad(argc, argv);
    }
    else if ( STRNEQ(subprogram, "edit", 4 ) )
    {
        if ( argc != 6 )
            PrintUsageAndExit();

        char * fileToEdit = argv[1];

        if ( !editWad.open(fileToEdit) )
        {
            printf("Creating %s...\n", fileToEdit);
            if ( !editWad.create(fileToEdit) )
                return EXIT_FAILURE;
        }

        for ( int i = 2; i < argc - 1; i++ )
        {
            char * arg = argv[i];

            if ( STRNEQ(arg, "-map", 4) )
            {
                strncpy(map.label, argv[i + 1], MAP_LABEL_LENGTH);
                // TODO: validate label format
                i++;
            }
            else if ( STRNEQ(arg, "-iwad", 5) )
            {
                if ( !resourceWad.open(argv[i + 1]) )
                {
                    fprintf(stderr,
                            "Error: could not open resource WAD %s\n",
                            argv[i + 1]);
                    return EXIT_FAILURE;
                }

                printf("Editing '%s' using resource IWAD '%s'\n",
                       fileToEdit,
                       argv[i + 1]);
                i++;
            }
        }
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow("",
                                           SDL_WINDOWPOS_CENTERED,
                                           SDL_WINDOWPOS_CENTERED,
                                           800,
                                           800,
                                           SDL_WINDOW_RESIZABLE);

    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);

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
                default:
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        
        SDL_Delay(15);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
