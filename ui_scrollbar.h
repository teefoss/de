//
//  ui_scrollbar.h
//  de
//
//  Created by Thomas Foster on 7/27/23.
//

#ifndef ui_scrollbar_h
#define ui_scrollbar_h

#include <stdbool.h>

typedef struct {
    enum { SCROLLBAR_HORIZONTAL, SCROLLBAR_VERTICAL } type;
    int location; // The row or col of the scroll bar.

    // The span of the scroll bar. (x if horizontal, y if vertical)
    int max; // in text coordinates
    int min;

    bool isDragging;

    int scrollPosition; // The y position visible at the top of the panel.
    int maxScrollPosition; // The maximum y position that can be scrolled to.
} Scrollbar;

/// x, y is the click point in text coordinates
/// - returns: the position or -1 if x, y is outside the scrollbar.
int GetPositionInScrollbar(const Scrollbar * scrollbar, int x, int y);

/// Update the scrollbar's scroll position. `textPosition`is the text
/// coordinate within the scrollbar to use to set it.
void ScrollToPosition(Scrollbar * scrollbar, int textPosition);

int GetScrollbarHandlePosition(Scrollbar * scrollbar, int textPosition);

#endif /* ui_scrollbar_h */
