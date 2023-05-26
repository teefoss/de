//
//  geometry.h
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#ifndef geometry_h
#define geometry_h

#include <stdbool.h>
#include <SDL2/SDL.h>

SDL_Rect MakeCenteredRect(const SDL_Point * point, int size);

/// Check if a line from `p1` to `p2` intersects with a line from `p3` to `p4`.
bool LineLineIntersection(const SDL_Point * p1,
                          const SDL_Point * p2,
                          const SDL_Point * p3,
                          const SDL_Point * p4);

/// Check if a line from `p1` to `p2` is partially or complete inside rect `r`.
bool LineInRect(const SDL_Point * p1, const SDL_Point * p2, const SDL_Rect * r);

#endif /* geometry_h */
