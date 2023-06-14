//
//  text.c
//  de
//
//  Created by Thomas Foster on 5/26/23.
//

#include "text.h"
#include "common.h"
#include "cp437.h"

#define TEXT_SCALE 1.0f

BufferCell GetCell(u16 data)
{
    BufferCell cell;
    cell.character  = (data & 0x00FF);
    cell.foreground = (data & 0x0F00) >> 8;
    cell.background = (data & 0xF000) >> 12;

    return cell;
}

void RenderChar(int x, int y, unsigned char character)
{
    SDL_RenderSetScale(renderer, TEXT_SCALE, TEXT_SCALE);

    // Scale drawing but not coordinates.
    int unscaledX = (float)x / TEXT_SCALE;
    int unscaledY = (float)y / TEXT_SCALE;

    const int w = 8;
    const int h = 16;

    const u8 * data = &cp437[character * 16];
    int bit = 7;

    for ( int row = 0; row < h; row++ )
    {
        for ( int col = 0; col < w; col++ )
        {
            if ( *data & (1 << bit) )
                SDL_RenderDrawPoint(renderer, unscaledX + col, unscaledY + row);

            if ( --bit < 8 - w )
            {
                ++data;
                bit = 7;
            }
        }
    }

    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
}

// TODO: use a global or static buffer and only resize when needed.
int RenderString(int x, int y, const char * format, ...)
{
    va_list args[2];
    va_start(args[0], format);
    va_copy(args[1], args[0]);

    int len = vsnprintf(NULL, 0, format, args[0]);
    char * buffer = calloc(len + 1, sizeof(*buffer));
    vsnprintf(buffer, len + 1, format, args[1]);
    va_end(args[0]);
    va_end(args[1]);

    const char * c = buffer;
    int x1 = x;
    int y1 = y;
    int w = 8 * TEXT_SCALE;
    int h = 16 * TEXT_SCALE;
    int tabSize = 4;

    while ( *c ) {
        switch ( *c ) {
            case '\n':
                y1 += h;
                x1 = x;
                break;
            case '\t':
                while ( (++x1 - x) % (tabSize * w) != 0 )
                    ;
                break;
            default:
                RenderChar(x1, y1, *c);
                x1 += w;
                break;
        }

        c++;
    }

    free(buffer);
    return x1;
}
