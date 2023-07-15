//
//  e_undo.c
//  de
//
//  Created by Thomas Foster on 7/14/23.
//

#include "e_undo.h"
#include "m_map.h"

#define MAX_UNDO_STATES 256

typedef struct
{
    Map states[MAX_UNDO_STATES];
    int current;
    int numStates;
} History;

static History undo;
static History redo;

/// Overwrite `destination` with a copy of `source`. Map arrays in
/// `destination` are freed if necessary.
static void CopyMap(Map * destination, Map * source)
{
    ASSERT(destination != NULL);
    ASSERT(source != NULL);

    if ( destination->vertices != NULL )
        FreeArray(destination->vertices);

    if ( destination->lines != NULL )
        FreeArray(destination->lines);

    if ( destination->things != NULL )
        FreeArray(destination->things);

    *destination = *source;

    destination->vertices = DeepCopy(source->vertices);
    destination->lines = DeepCopy(source->lines);
    destination->things = DeepCopy(source->things);
}

static void PushToHistory(History * hist)
{
    CopyMap(&hist->states[hist->current], &map);
    hist->current = (hist->current + 1) % MAX_UNDO_STATES;

    if ( hist->numStates < MAX_UNDO_STATES )
        hist->numStates++;
}

static void PopFromHistory(History * hist)
{
    hist->current = (hist->current + MAX_UNDO_STATES - 1) % MAX_UNDO_STATES;
    CopyMap(&map, &hist->states[hist->current]);
    hist->numStates--;
}

void SaveUndoState(void)
{
    PushToHistory(&undo);

    // Clear redo history when making a change.
    redo.numStates = 0;
    redo.current = 0;
}

void Undo(void)
{
    if ( undo.numStates == 0 )
        return;

    PushToHistory(&redo);
    PopFromHistory(&undo);
}

void Redo(void)
{
    if ( redo.numStates == 0 )
        return;

    PushToHistory(&undo);
    PopFromHistory(&redo);
}
