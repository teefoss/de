//
//  text.h
//  de
//
//  Created by Thomas Foster on 5/26/23.
//

#ifndef text_h
#define text_h

#include "common.h"

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

typedef struct
{
    u8 character;
    u8 foreground;
    u8 background;
} BufferCell;

BufferCell GetCell(u16 data);
void PrintChar(int x, int y, unsigned char character);
int PrintString(int x, int y, const char * format, ...);

#endif /* text_h */
