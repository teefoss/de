//
//  geometry.c
//  de
//
//  Created by Thomas Foster on 5/22/23.
//

#include "geometry.h"

bool PointsEqual(const SDL_Point * a, const SDL_Point * b)
{
    return a->x == b->x && a->y == b->y;
}

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

void EnclosePoint(const SDL_Point * point, Box * box)
{
    if ( point->x < box->left )
        box->left = point->x;

    if ( point->x > box->right )
        box->right = point->x;

    if ( point->y < box->top )
        box->top = point->y;

    if ( point->y > box->bottom )
        box->bottom = point->y;
}

SDL_Rect BoxToRect(const Box * box)
{
    SDL_Rect rect =
    {
        .x = box->left,
        .y = box->top,
        .w = box->right - box->left,
        .h = box->bottom - box->top
    };

    return rect;
}

SDL_Rect FitAndCenterRect(const SDL_Rect * rect,
                          const SDL_Rect * container,
                          int margin)
{
    float scale;
    int xOffset = 0;
    int yOffset = 0;

    if ( rect->w > rect->h )
    {
        scale = (float)container->w / rect->w;
        yOffset = (container->h - rect->h * scale) / 2.0f;
    }
    else
    {
        scale = (float)container->h /rect->h;
        xOffset = (container->w - rect->w * scale) / 2.0f;
    }

    SDL_Rect result = {
        .x = container->x + xOffset,
        .y = container->y + yOffset,
        .w = rect->w * scale,
        .h = rect->h * scale,
    };

    result.x += margin;
    result.y += margin;
    result.w -= margin * 2;
    result.h -= margin * 2;

    return result;
}

// Liang-Barsky function by Daniel White @
// http://www.skytopia.com/project/articles/compsci/clipping.html
// This function inputs 8 numbers, and outputs 4 new numbers (plus a boolean
// value to say whether the clipped line is drawn at all).

bool LiangBarsky (double edgeLeft,
                  double edgeRight,
                  double edgeBottom,
                  double edgeTop, // Define the x/y clipping values for the border.

                  // Define the start and end points of the line.
                  double x0src,
                  double y0src,
                  double x1src,
                  double y1src,

                  // The output values, so declare these outside.
                  double * x0clip,
                  double * y0clip,
                  double * x1clip,
                  double * y1clip)
{

    double t0 = 0.0;
    double t1 = 1.0;

    double xdelta = x1src - x0src;
    double ydelta = y1src - y0src;

    double p = 0.0;
    double q = 0.0;
    double r;

    // Traverse through left, right, bottom, top edges.
    for ( int edge = 0; edge < 4; edge++ )
    {
        if ( edge == 0 ) { p = -xdelta;  q = -(edgeLeft - x0src);   }
        if ( edge == 1 ) { p =  xdelta;  q =  (edgeRight - x0src);  }
        if ( edge == 2 ) { p =  ydelta;  q =  (edgeBottom - y0src); }
        if ( edge == 3 ) { p = -ydelta;  q = - (edgeTop - y0src);    }

        r = q / p;


        if ( p == 0 && q < 0 )
            return false;   // Don't draw line at all. (parallel line outside)

        if ( p < 0 )
        {
            if ( r > t1 )
                 return false;      // Don't draw line at all.
            else if ( r > t0 )
                t0 = r;             // Line is clipped!
        }
        else if ( p > 0 )
        {
            if ( r < t0 )
                return false;       // Don't draw line at all.
            else if ( r < t1 )
                t1 = r;             // Line is clipped!
        }
    }

    *x0clip = x0src + t0 * xdelta;
    *y0clip = y0src + t0 * ydelta;
    *x1clip = x0src + t1 * xdelta;
    *y1clip = y0src + t1 * ydelta;

    return true; // (clipped) line is drawn
}


typedef int OutCode;

const int INSIDE    = 0;
const int LEFT      = 1;
const int RIGHT     = 2;
const int BOTTOM    = 4;
const int TOP       = 8;

/// Compute the bit code for a point (x, y) using the clip rectangle, `rect`.
OutCode ComputeOutCode(const SDL_Rect * rect, double x, double y)
{
    OutCode code = INSIDE;  // initialised as being inside of clip window

    if (x < rect->x)           // to the left of clip window
        code |= LEFT;
    else if (x > rect->x + rect->w)      // to the right of clip window
        code |= RIGHT;
    if (y < rect->y)           // below the clip window
        code |= BOTTOM;
    else if (y > rect->y + rect->h)      // above the clip window
        code |= TOP;

    return code;
}

// Cohen–Sutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with
// diagonal from (xmin, ymin) to (xmax, ymax).
bool CohenSutherlandLineClip(const SDL_Rect * rect,
                             double * x0,
                             double * y0,
                             double * x1,
                             double * y1)
{
    // compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
    OutCode outcode0 = ComputeOutCode(rect, *x0, *y0);
    OutCode outcode1 = ComputeOutCode(rect, *x1, *y1);

    double xmin = rect->x;
    double xmax = rect->x + rect->w;
    double ymin = rect->y;
    double ymax = rect->y + rect->h;

    while (true) {
        if (!(outcode0 | outcode1)) {
            // bitwise OR is 0: both points inside window; trivially accept and exit loop
            return true;
        } else if (outcode0 & outcode1) {
            // bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
            // or BOTTOM), so both must be outside window; exit loop (accept is false)
            return false;
        } else {
            // failed both tests, so calculate the line segment to clip
            // from an outside point to an intersection with clip edge
            double x = 0.0, y = 0.0;

            // At least one endpoint is outside the clip rectangle; pick it.
            OutCode outcodeOut = outcode1 > outcode0 ? outcode1 : outcode0;

            // Now find the intersection point;
            // use formulas:
            //   slope = (y1 - y0) / (x1 - x0)
            //   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
            //   y = y0 + slope * (xm - x0), where xm is xmin or xmax
            // No need to worry about divide-by-zero because, in each case, the
            // outcode bit being tested guarantees the denominator is non-zero
            if (outcodeOut & TOP) {           // point is above the clip window
                x = *x0 + (*x1 - *x0) * (ymax - *y0) / (*y1 - *y0);
                y = ymax;
            } else if (outcodeOut & BOTTOM) { // point is below the clip window
                x = *x0 + (*x1 - *x0) * (ymin - *y0) / (*y1 - *y0);
                y = ymin;
            } else if (outcodeOut & RIGHT) {  // point is to the right of clip window
                y = *y0 + (*y1 - *y0) * (xmax - *x0) / (*x1 - *x0);
                x = xmax;
            } else if (outcodeOut & LEFT) {   // point is to the left of clip window
                y = *y0 + (*y1 - *y0) * (xmin - *x0) / (*x1 - *x0);
                x = xmin;
            }

            // Now we move outside point to intersection point to clip
            // and get ready for next pass.
            if (outcodeOut == outcode0) {
                *x0 = x;
                *y0 = y;
                outcode0 = ComputeOutCode(rect, *x0, *y0);
            } else {
                *x1 = x;
                *y1 = y;
                outcode1 = ComputeOutCode(rect, *x1, *y1);
            }
        }
    }
}
