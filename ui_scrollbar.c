//
//  ui_scrollbar.c
//  de
//
//  Created by Thomas Foster on 7/27/23.
//

#include "ui_scrollbar.h"
#include "common.h"


int GetPositionInScrollbar(const Scrollbar * scrollbar, int x, int y)
{
    bool aligned;
    bool inside;

    if ( scrollbar->type == SCROLLBAR_VERTICAL )
    {
        aligned = x == scrollbar->location;
        inside = y >= scrollbar->min && y <= scrollbar->max;

        if ( aligned && inside )
            return y - scrollbar->min;
    }
    else
    {
        aligned = y == scrollbar->location;
        inside = x >= scrollbar->min && x <= scrollbar->max;

        if ( aligned && inside )
            return x - scrollbar->min;
    }

    return -1;
}



void ScrollToPosition(Scrollbar * scrollbar, int textPosition)
{
    int range = scrollbar->max - scrollbar->min;
    float percent = (float)(textPosition - scrollbar->min) / range;
    scrollbar->scrollPosition = scrollbar->maxScrollPosition * percent;

    CLAMP(scrollbar->scrollPosition, 0, scrollbar->maxScrollPosition);
}



int GetScrollbarHandlePosition(Scrollbar * scrollbar, int textPosition)
{
    int handlePosition = textPosition - scrollbar->min;
    SDL_clamp(handlePosition, scrollbar->min, scrollbar->max);

    return handlePosition;
}
