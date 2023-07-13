//
//  edit.h
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#ifndef edit_h
#define edit_h

#include "wad.h"
#include <stdbool.h>

#define DOOM1_PATH "doom1/"
#define DOOMSE_PATH "doomSE/"
#define DOOM2_PATH "doom2/"

typedef enum
{
    GAME_DOOM1,
    GAME_DOOM1SE,
    GAME_DOOM2
} Game;

typedef struct
{
    Game game;  // which game the level is for.
    Wad * iwad; // aka "resource" WAD.
    Wad * pwad; // user-created WAD being edited.
} Editor;

extern Editor editor;

void InitEditor(void);
void EditorLoop(void);
void CleanupEditor(void);
void RenderEditor(void);
void DeselectAllObjects(void);

#endif /* edit_h */
