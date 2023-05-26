//
//  geometry.c
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#include "geometry.h"

SDL_Rect MakeCenteredRect(const SDL_Point * point, int size)
{
    SDL_Rect square = {
        .x = point->x - size / 2,
        .y = point->y - size / 2,
        .w = size,
        .h = size
    };

    return square;
}

bool LineLineIntersection(const SDL_Point * p1,
                          const SDL_Point * p2,
                          const SDL_Point * p3,
                          const SDL_Point * p4)
{
    // Calculate the direction of the lines.
    float denominator = (p4->y - p3->y) * (p2->x - p1->x) - (p4->x - p3->x) * (p2->y - p1->y);

    // Check if lines are parallel and vertically aligned.
    if ( denominator == 0 && p1->x == p2->x && p3->x == p4->x ) {
        return false;
    }

    // Calculate the 'u' parameters.
    float uA = ((p4->x - p3->x) * (p1->y - p3->y) - (p4->y - p3->y) * (p1->x - p3->x)) / denominator;
    float uB = ((p2->x - p1->x) * (p1->y - p3->y) - (p2->y - p1->y) * (p1->x - p3->x)) / denominator;

    // If uA and uB are between 0-1, lines are colliding
    return uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1;
}

bool LineInRect(const SDL_Point * p1, const SDL_Point * p2, const SDL_Rect * r)
{
    if ( SDL_PointInRect(p1, r) && SDL_PointInRect(p2, r) )
        return true;

    // The rect's corners.
    SDL_Point ul    = { r->x, r->y };
    SDL_Point ur    = { r->x + r->w, r->y };
    SDL_Point ll    = { r->x, r->y + r->h };
    SDL_Point lr    = { r->x + r->w, r->y + r->h };

    // Check for line intersection with the rect's sides.
    bool left       = LineLineIntersection(p1, p2, &ul, &ll);
    bool right      = LineLineIntersection(p1, p2, &ur, &lr);
    bool top        = LineLineIntersection(p1, p2, &ul, &ur);
    bool bottom     = LineLineIntersection(p1, p2, &ll, &lr);

    return left || right || top || bottom;
}
