//
//  vertex.h
//  de
//
//  Created by Thomas Foster on 6/27/23.
//

#ifndef vertex_h
#define vertex_h

#include <stdbool.h>

typedef struct
{
    SDL_Point origin;
    bool selected;
    int referenceCount;
    bool removed;
} Vertex;

#endif /* vertex_h */
