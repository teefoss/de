//
//  sector.c
//  de
//
//  Created by Thomas Foster on 6/21/23.
//

#include "sector.h"
#include "map.h"
#include "doomdata.h"
#include <errno.h>

#ifdef DRAW_BLOCK_MAP
SDL_Window * bmapWindow;
SDL_Renderer * bmapRenderer;
SDL_Texture * bmapTexture;

SDL_Rect bmapLocation;
float bmapScale = 1.0f;

static SDL_Color black = { 0, 0, 0, 255 };
static SDL_Color red = { 255, 0, 0, 255 };
static SDL_Color magenta = { 255, 0, 255, 255 };

// Assume target is set to bmapTexture!
static void DrawPixel(int x, int y, SDL_Color c)
{
    SDL_SetRenderDrawColor(bmapRenderer, c.r, c.g, c.b, c.a);
    SDL_RenderDrawPoint(bmapRenderer, x, y);
}

// Force immediate render of the block map texture
static void Refresh(void)
{
    SDL_SetRenderTarget(bmapRenderer, NULL);
    SDL_RenderClear(bmapRenderer);
    SDL_RenderCopy(bmapRenderer, bmapTexture, NULL, NULL);
    SDL_RenderPresent(bmapRenderer);
    SDL_SetRenderTarget(bmapRenderer, bmapTexture);
    SDL_PumpEvents();
}
#endif


#define BLOCK_MAP_RESOLUTION 8
#define BACK_SIDE_FLAG 0x8000
#define BLOCK(x, y) (blockMap + y * blockMapWidth + x)

typedef enum { NO_DIRECTION, NORTH, EAST, SOUTH, WEST } Direction;

typedef struct
{
    u16 north;
    u16 east;
    u16 south;
    u16 west;
    u16 northWest;
    u16 northEast;
    u16 southWest;
    u16 southEast;
    bool visited;
    bool skip;
} Block;

static SDL_Rect mapBounds;
static Block * blockMap;
static int blockMapWidth;
static int blockMapHeight;


static void DrawBlockMapLine(int lineIndex)
{
    Line * line = Get(map.lines, lineIndex);
    Vertex * vertices = map.vertices->data;

    int x1 = (vertices[line->v1].origin.x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y1 = (vertices[line->v1].origin.y - mapBounds.y) / BLOCK_MAP_RESOLUTION;
    int x2 = (vertices[line->v2].origin.x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y2 = (vertices[line->v2].origin.y - mapBounds.y) / BLOCK_MAP_RESOLUTION;

    int left, right, top, bottom;

    if ( x1 == x2 ) // Vertical line
    {
//        printf("vertical line: (%d, %d) - (%d, %d)\n", x1, y1, x2, y2);
        left = right = lineIndex + 1;

        if ( y1 > y2 )
        {
            left |= BACK_SIDE_FLAG;
        }
        else
        {
            right |= BACK_SIDE_FLAG;
            int temp = y1;
            y1 = y2;
            y2 = temp;
        }

        BLOCK(x1, y1)->skip = true;

        while ( y1 > y2 )
        {
//            printf("- %d, %d\n", x1, y1);
            Block * block = BLOCK(x1, y1);
#ifdef DRAW_BLOCK_MAP
            DrawPixel(x1, y1, block->skip ? magenta : black);
#endif
            block->west = left;
            block->east = right;
            y1--;
        }

        BLOCK(x1, y1)->skip = true;
#ifdef DRAW_BLOCK_MAP
        DrawPixel(x1, y2, magenta);
#endif

        return;
    }

    if ( y1 == y2 ) // Horizontal line
    {
        top = bottom = lineIndex + 1;

        if ( x1 < x2 )
        {
            top |= BACK_SIDE_FLAG;
        }
        else
        {
            bottom |= BACK_SIDE_FLAG;
            int temp = x1;
            x1 = x2;
            x2 = temp;
        }

        BLOCK(x1, y1)->skip = true;

        while ( x1 < x2 )
        {
            Block * block = BLOCK(x1, y1);
#ifdef DRAW_BLOCK_MAP
            DrawPixel(x1, y1, block->skip ? magenta : black);
#endif
            block->north = top;
            block->south = bottom;
            x1++;
        }

        BLOCK(x2, y1)->skip = true;
#ifdef DRAW_BLOCK_MAP
        DrawPixel(x2, y1, magenta);
#endif

        return;
    }

    // Sloping line:

    int index = lineIndex + 1;
    int ne = 0;
    int nw = 0;
    int se = 0;
    int sw = 0;

//    printf("sloping line: (%d, %d) - (%d, %d)\n", x1, y1, x2, y2);

    if ( x1 < x2 )
    {
        if ( y1 < y2 ) // NW-SE line, font side is SW
        {
            sw = index;
            ne = index | BACK_SIDE_FLAG;
        }
        else // NE-SW line, front is SE
        {
            se = index;
            nw = index | BACK_SIDE_FLAG;
        }
    }
    else // x2 < x1
    {
        if ( y1 < y2 ) // NE-SW line, front is NW
        {
            nw = index;
            se = index | BACK_SIDE_FLAG;
        }
        else // NW-SE line, front is NE
        {
            ne = index;
            sw = index | BACK_SIDE_FLAG;
        }
    }

    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int error = dx + dy;
    int error2;

    int x = x1;
    int y = y1;

    BLOCK(x1, y1)->skip = true;

    while ( x != x2 || y != y2 )
    {
        Block * block = BLOCK((int)x, (int)y);
#ifdef DRAW_BLOCK_MAP
        DrawPixel(x, y, block->skip ? magenta : black);
#endif
//        printf("- %d, %d\n", x, y);

        block->northEast = ne;
        block->southWest = sw;
        block->northWest = nw;
        block->southEast = se;

        error2 = error * 2;

        if ( error2 >= dy )
        {
            error += dy;
            x += sx;
        }

        if ( error2 <= dx )
        {
            error += dx;
            y += sy;
        }
    }

    BLOCK(x2, y2)->skip = true;
#ifdef DRAW_BLOCK_MAP
    DrawPixel(x2, y2, magenta);
#endif

//    printf("- %d, %d\n", x2, y2);
}

static void CreateBlockMap(void)
{
    mapBounds = GetMapBounds();

    // TODO: memset 0 and only realloc when needed.
    if ( blockMap )
        free(blockMap);

    blockMapWidth = mapBounds.w / BLOCK_MAP_RESOLUTION;
    blockMapHeight = mapBounds.h / BLOCK_MAP_RESOLUTION;
    printf("block map size: %d x %d\n", blockMapWidth, blockMapHeight);

    errno = 0;
    size_t allocSize = blockMapWidth * blockMapHeight * sizeof(*blockMap);
    blockMap = calloc(blockMapWidth * blockMapHeight, sizeof(*blockMap));

    if ( blockMap == NULL )
    {
        fprintf(stderr, "failed to allocate block map: %s\n", strerror(errno));
        return;
    }

    printf("allocated %zu bytes for block map\n", allocSize);

#ifdef DRAW_BLOCK_MAP
    if ( bmapWindow == NULL ) {
        bmapWindow = SDL_CreateWindow("", 0, 0, blockMapWidth * 2, blockMapHeight * 2, 0);
        if ( bmapWindow == NULL )
            fprintf(stderr, "could not create bmap window: %s\n", SDL_GetError());
        bmapRenderer = SDL_CreateRenderer(bmapWindow, -1, SDL_RENDERER_SOFTWARE|SDL_RENDERER_TARGETTEXTURE);
        if ( bmapRenderer == NULL )
            fprintf(stderr, "could not create bmap renderer: %s\n", SDL_GetError());
//        bmapRenderer = renderer;
        bmapTexture = SDL_CreateTexture(bmapRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, blockMapWidth, blockMapHeight);
        if ( bmapTexture == NULL )
            fprintf(stderr, "could not create bmap texture: %s\n", SDL_GetError());

    }

    SDL_SetRenderTarget(bmapRenderer, bmapTexture);
    SDL_SetRenderDrawColor(bmapRenderer, 255, 255, 255, 255);
    SDL_RenderClear(bmapRenderer);
#endif

    Line * line = map.lines->data;
    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        if ( line->selected != DELETED )
            DrawBlockMapLine(i);
    }

#ifdef DRAW_BLOCK_MAP
    SDL_SetRenderTarget(bmapRenderer, NULL);
#endif
}

static void SelectLine(int index)
{
    Line * lines = map.lines->data;
    index--;

    if ( index & BACK_SIDE_FLAG )
    {
        index &= ~BACK_SIDE_FLAG;
        lines[index].selected = BACK_SELECTED;
    }
    else
    {
        lines[index].selected = FRONT_SELECTED;
    }
}


static void FloodFill2(int x, int y, Direction direction)
{
    if ( x < 0 || x >= blockMapWidth || y < 0 || y >= blockMapHeight )
    {
        fprintf(stderr, "Fill point out of bounds! (%d, %d)\n", x, y);
        return;
    }

    Block * block = BLOCK(x, y);

    if ( block->skip )
        return;

    if ( block->visited )
        return;

    switch ( direction )
    {
        case NO_DIRECTION:
            break;
        case NORTH: {
            if ( block->south )
                return SelectLine(block->south);
            else if ( block->southEast )
                return SelectLine(block->southEast);
            else if ( block->southWest )
                return SelectLine(block->southWest);
            break;
        }
        case SOUTH: {
            if ( block->north )
                return SelectLine(block->north);
            else if ( block->northEast )
                return SelectLine(block->northEast);
            else if ( block->northWest )
                return SelectLine(block->northWest);
            break;
        }
        case EAST:
            if ( block->west )
                return SelectLine(block->west);
            else if ( block->northWest )
                return SelectLine(block->northWest);
            else if ( block->southWest )
                return SelectLine(block->southWest);
            break;
        case WEST:
            if ( block->east )
                return SelectLine(block->east);
            else if ( block->northEast )
                return SelectLine(block->northEast);
            else if ( block->southEast )
                return SelectLine(block->southEast);
            break;
    }

    block->visited = true;

#ifdef DRAW_BLOCK_MAP
    DrawPixel(x, y, red);
    Refresh();
#endif

    FloodFill2(x + 1, y, EAST);
    FloodFill2(x - 1, y, WEST);
    FloodFill2(x, y - 1, NORTH);
    FloodFill2(x, y + 1, SOUTH);
}

void SelectSector(const SDL_Point * point)
{
    CreateBlockMap();
    DeselectAllObjects();

    int x = (point->x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y = (point->y - mapBounds.y) / BLOCK_MAP_RESOLUTION;

#ifdef DRAW_BLOCK_MAP
    SDL_SetRenderTarget(bmapRenderer, bmapTexture);
#endif

    FloodFill2(x, y, NO_DIRECTION);

#ifdef DRAW_BLOCK_MAP
    SDL_SetRenderTarget(bmapRenderer, NULL);
#endif
}
