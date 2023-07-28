//
//  thing.h
//  de
//
//  Created by Thomas Foster on 6/18/23.
//

#ifndef thing_h
#define thing_h

#include "e_editor.h"
#include "g_patch.h"

typedef enum
{
    // Options are placed first so they mirror the actual MTF_ flags.
    THING_PROPERTY_EASY,
    THING_PROPERTY_MEDIUM,
    THING_PROPERTY_HARD,
    THING_PROPERTY_AMBUSH,
    THING_PROPERTY_NETWORK,

    THING_PROPERTY_ANGLE,
    THING_PROPERTY_TYPE,
} ThingProperty;

typedef struct {
    SDL_Point origin;
    int angle;
    int type;
    int options;

    bool selected;
    bool deleted;
} Thing;

#endif /* thing_h */
