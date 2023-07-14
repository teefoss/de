//
//  defaults.h
//  de
//
//  Created by Thomas Foster on 6/21/23.
//

#ifndef defaults_h
#define defaults_h

#include <SDL2/SDL.h>

#define NUM_PALETTE_COLORS 16

typedef struct
{
    const char * name;
    int * location;
    enum {
        FORMAT_COMMENT, // A dummy default, used to write a comment when saving.
        FORMAT_DECIMAL,
        FORMAT_HEX
    } format;
} Default;

typedef enum
{   // TODO: namespace these!
    BACKGROUND,
    GRID_LINES,
    GRID_TILES,
    SELECTION,
    VERTEX,
    LINE_ONE_SIDED,
    DEF_COLOR_LINE_TWO_SIDED,
    DEF_COLOR_LINE_SPECIAL,
    SELECTION_BOX,

    THING_PLAYER,
    THING_DEMON,
    THING_WEAPON,
    THING_PICKUP,
    THING_DECOR,
    THING_GORE,
    THING_OTHER,

    NUM_COLORS,
} Color;

extern int palette[];
extern int colors[];

SDL_Color DefaultColor(int i);
void LoadDefaults(const char * fileName);

#endif /* defaults_h */
