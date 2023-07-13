//
//  edit_state.c
//  de
//
//  Created by Thomas Foster on 5/30/23.
//

#include "edit_state.h"

void UpdateEdit(float dt);
void UpdateAutoscroll(float dt);
void UpdateSelectionBox(float dt);
void UpdateDragView(float dt);
void DragSelectedObjects(float dt);
void UpdateNewLine(float dt);

void ProcessEditEvent(const SDL_Event * event);
void HandleDragBoxEvent(const SDL_Event * event);
void HandleDragViewEvent(const SDL_Event * event);
void HandleDragObjectsEvent(const SDL_Event * event);
void ProcessNewLineEvent(const SDL_Event * event);

EditorStateID editorState;

EditorState states[] = {
    [ES_EDIT] = { ProcessEditEvent, UpdateEdit },
    [ES_AUTO_SCROLL] = { NULL, UpdateAutoscroll },
    [ES_DRAG_BOX] = { HandleDragBoxEvent, UpdateSelectionBox },
    [ES_DRAG_VIEW] = { HandleDragViewEvent, UpdateDragView },
    [ES_DRAG_OBJECTS] = { HandleDragObjectsEvent, DragSelectedObjects },
    [ES_NEW_LINE] = { ProcessNewLineEvent, UpdateNewLine },
};


void StateUpdate(float dt)
{
    if ( states[editorState].update )
        states[editorState].update(dt);
}

void StateHandleEvent(const SDL_Event * event)
{
    if ( states[editorState].handleEvent )
        states[editorState].handleEvent(event);
}
