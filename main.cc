//
//  main.cc
//  de
//
//  Created by Thomas Foster on 5/19/23.
//

#include "WadFile.h"
#include "Map.h"

#include <SDL2/SDL.h>
#include <string.h>

// de file.wad -r doom.wad

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

int main(int argc, char ** argv)
{
    printf("de\n"
           "[d]oom [e]ditor\n"
           "v. 0.1 Copyright (C) 2023 Thomas Foster\n\n");

    argc--;
    argv++;

    if ( argc < 2 ) {
        PrintUsageAndExit();
    }

    WadFile editWad;
    WadFile resourceWad;
    Map map;

    // Parse arguments for each subprogram.
    if ( STRNEQ(argv[0], "list", 4) )
    {
        WadFile wad;

        if ( wad.open(argv[1]) )
        {
            printf("%s\n", argv[1]);
            wad.listDirectory();
            return EXIT_SUCCESS;
        }
        else
        {
            return EXIT_FAILURE;
        }
    }
    else if ( STRNEQ(argv[0], "edit", 4 ) )
    {
        if ( argc != 6 )
        {
            PrintUsageAndExit();
        }

        if ( !editWad.open(argv[1]) )
        {
            if ( !editWad.create(argv[1]) )
            {
                return EXIT_FAILURE;
            }
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
                       argv[1],
                       argv[i + 1]);
                i++;
            }
        }
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow
    (   "",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        800,
        SDL_WINDOW_RESIZABLE );

    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);

    bool run = true;
    while ( run ) {
        SDL_Event event;
        while ( SDL_PollEvent(&event) ) {
            switch ( event.type ) {
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
