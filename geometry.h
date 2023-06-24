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

typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
} Box;

SDL_Rect MakeCenteredRect(const SDL_Point * point, int size);

/// Check if a line from `p1` to `p2` intersects with a line from `p3` to `p4`.
bool LineLineIntersection(const SDL_Point * p1,
                          const SDL_Point * p2,
                          const SDL_Point * p3,
                          const SDL_Point * p4);

/// Check if a line from `p1` to `p2` is partially or complete inside rect `r`.
bool LineInRect(const SDL_Point * p1, const SDL_Point * p2, const SDL_Rect * r);

void EnclosePoint(const SDL_Point * point, Box * box);
SDL_Rect BoxToRect(const Box * box);

/// Expand or shrink `rect` to fit in `container`, centered, keeping its
/// aspect ratio correct. `rect` will be inset by `margin` pixels.
SDL_Rect FitAndCenterRect(const SDL_Rect * rect,
                          const SDL_Rect * container,
                          int margin);

#endif /* geometry_h */
