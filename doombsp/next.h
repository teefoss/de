//
//  next.h
//  de
//
//  Created by Thomas Foster on 7/5/23.
//
// NeXTSTEP Types
//

#ifndef next_h
#define next_h

typedef float NXCoord;

typedef struct _NXPoint
{
    NXCoord x;
    NXCoord y;
} NXPoint;

typedef struct _NXSize
{
    NXCoord width;
    NXCoord height;
} NXSize;

typedef struct _NXRect
{
    NXPoint origin;
    NXSize size;
} NXRect;

#endif /* next_h */
