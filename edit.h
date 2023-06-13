//
//  edit.h
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#ifndef edit_h
#define edit_h

#include "wad.h"

#define DOOM1_PATH "doom1/"
#define DOOMSE_PATH "doomSE/"
#define DOOM2_PATH "doom2/"

typedef enum
{
    GAME_DOOM1,
    GAME_DOOM1SE,
    GAME_DOOM2
} GameType;

extern Wad * resourceWad;

void EditorLoop(void);
void RenderEditor(void);

#endif /* edit_h */
