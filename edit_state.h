//
//  edit_state.h
//  de
//
//  Created by Thomas Foster on 5/30/23.
//

#ifndef edit_state_h
#define edit_state_h

#include <SDL2/SDL.h>

typedef enum
{
    ES_EDIT,
    ES_AUTO_SCROLL,
    ES_DRAG_BOX,
    ES_DRAG_OBJECTS,
    ES_DRAG_VIEW,
    ES_ZOOM,
} EditorStateID;

typedef struct
{
    void (* handleEvent)(const SDL_Event *);
    void (* update)(float);
} EditorState;

extern EditorStateID editorState;

void StateUpdate(float dt);
void StateHandleEvent(const SDL_Event * event);

#endif /* edit_state_h */
