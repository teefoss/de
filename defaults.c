//
//  defaults.c
//  de
//
//  Created by Thomas Foster on 6/21/23.
//

#include "defaults.h"
#include "thing.h"

//    { 0x04, 0x14, 0x41 } funkly blue

int palette[16] = {
    0x000000,
    0x0000AA,
    0x00AA00,
    0x00AAAA,
    0xAA0000,
    0xAA00AA,
    0xAA5500,
    0xAAAAAA,
    0x555555,
    0x5555FF,
    0x55FF55,
    0x55FFFF,
    0xFF5555,
    0xFF55FF,
    0xFFFF55,
    0xFFFFFF,
};

#define LIGHT_MODE
#ifdef LIGHT_MODE
#elif
// TODO: dark mode
#endif

int colors[NUM_COLORS] =
{
    [BACKGROUND]        = 0xFFFFFF,
    [GRID_LINES]        = 0xE8E8E8,
    [GRID_TILES]        = 0xD0D0D0,
    [SELECTION]         = 0xFF0000,
    [LINE_ONE_SIDED]    = 0x000000,
    [LINE_TWO_SIDED]    = 0x0000FF,
    [LINE_SPECIAL]      = 0xFF00FF,

    [THING_PLAYER]      = 0x55FF55, // light green
    [THING_DEMON]       = 0x000000, // black
    [THING_WEAPON]      = 0xAA5500, // brown
    [THING_PICKUP]      = 0xFF55FF, // light purple
    [THING_DECOR]       = 0x555555, // dark gray
    [THING_GORE]        = 0xAAAAAA, // light gray
    [THING_OTHER]       = 0x5555FF, // light blue
};

#define PALETTE_DEFAULT(x) { "PALETTE_" #x, &palette[x], FORMAT_HEX }
#define COLOR_DEFAULT(e) { #e, &colors[e], FORMAT_HEX }

static int numDefaults;

Default defaults[] =
{
    { "\n; PANEL COLORS\n\n", NULL, FORMAT_COMMENT },

    PALETTE_DEFAULT(0),
    PALETTE_DEFAULT(1),
    PALETTE_DEFAULT(2),
    PALETTE_DEFAULT(3),
    PALETTE_DEFAULT(4),
    PALETTE_DEFAULT(5),
    PALETTE_DEFAULT(6),
    PALETTE_DEFAULT(7),
    PALETTE_DEFAULT(8),
    PALETTE_DEFAULT(9),
    PALETTE_DEFAULT(10),
    PALETTE_DEFAULT(11),
    PALETTE_DEFAULT(12),
    PALETTE_DEFAULT(13),
    PALETTE_DEFAULT(14),
    PALETTE_DEFAULT(15),

    { "\n; EDITOR COLORS\n\n", NULL, FORMAT_COMMENT },

    COLOR_DEFAULT(BACKGROUND),
    COLOR_DEFAULT(GRID_LINES),
    COLOR_DEFAULT(GRID_TILES),
    COLOR_DEFAULT(SELECTION),
    COLOR_DEFAULT(VERTEX),
    COLOR_DEFAULT(LINE_ONE_SIDED),
    COLOR_DEFAULT(LINE_TWO_SIDED),
    COLOR_DEFAULT(LINE_SPECIAL),
    COLOR_DEFAULT(THING_PLAYER),
    COLOR_DEFAULT(THING_DEMON),
    COLOR_DEFAULT(THING_WEAPON),
    COLOR_DEFAULT(THING_PICKUP),
    COLOR_DEFAULT(THING_DECOR),
    COLOR_DEFAULT(THING_GORE),
    COLOR_DEFAULT(THING_OTHER),
};

#pragma mark -

SDL_Color DefaultColor(int i)
{
    return Int24ToSDL(colors[i]);
}

#pragma mark - CONFIG FILE

static void StripComments(char * line)
{
    char * c = line;

    while ( *c ) {
        if ( *c == ';' ) {
            *c = '\n';
            *(c + 1) = '\0';
            return;
        }
        c++;
    }
}

static bool ParseLine(char * line)
{
    StripComments(line);

    if ( line[0] == '\n' )
        return true;

    char name[80];
    int value;

    if ( sscanf(line, "%s %i\n", name, &value) != 2 )
        return false;

    Capitalize(name);

    for ( int i = 0; i < numDefaults; i++ )
    {
        if ( strncmp(name, defaults[i].name, sizeof(name)) == 0 )
        {
            *defaults[i].location = value;
            return true;
        }
    }

    fprintf(stderr, "Bad config name!\n");
    return false;
}

static void ParseDefaults(FILE * file)
{
    char line[512] = { 0 };
    int lineNum = 0;

    while ( fgets(line, sizeof(line), file) != NULL )
    {
        ++lineNum;
        if ( !ParseLine(line) )
            fprintf(stderr, "Error reading config on line %d!\n", lineNum);
    }
}

// TODO: Parse line by line, only rewriting defaults present in the file.
void SaveDefaults(const char * fileName)
{
    FILE * file = fopen(fileName, "w");

    if ( file == NULL )
    {
        fprintf(stderr, "Could not create config file '%s'!\n", fileName);
        return;
    }

    for ( int i = 0; i < numDefaults; i++ )
    {
        Default * def = &defaults[i];

        if ( def->format == FORMAT_COMMENT )
        {
            fprintf(file, "%s", def->name);
            continue;
        }

        char name[80] = { 0 };
        strncpy(name, def->name, sizeof(name));

        for ( char * c = name; *c; c++ )
            *c = tolower(*c);

        int count = fprintf(file, "%s ", name);
        while ( count++ < 20 )
            fprintf(file, " ");

        switch ( def->format )
        {
            case FORMAT_DECIMAL:
                fprintf(file, "%d\n", *def->location);
                break;
            case FORMAT_HEX:
                fprintf(file, "0x%06X\n", *def->location);
            default:
                break;
        }
    }

    fclose(file);
}

void LoadDefaults(const char * fileName)
{
    numDefaults = sizeof(defaults) / sizeof(defaults[0]);

    FILE * file = fopen(fileName, "r");

    if ( file )
    {
        printf("Loading config file '%s'...\n", fileName);
        ParseDefaults(file);
        fclose(file);
    }
    else
    {
        printf("Creating config file '%s'...\n", fileName);
        SaveDefaults(fileName);
    }
}
