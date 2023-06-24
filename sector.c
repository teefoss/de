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
float bmapScale = 2.0f;

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

// Block Flags
#define SOUTH_FRONT     1 // north-facing side is the front
#define WEST_FRONT      2 // west-facing side is the front
#define VISITED         4 // visited during flood fill
#define SKIP            8 // ignore this block (vertexes)

typedef struct
{
    u16 lineIndex;
    u8 flags;
} Block;

static SDL_Rect mapBounds;
static Block * blockMap;
static int blockMapWidth;
static int blockMapHeight;
static size_t allocated;


static void DrawBlockMapLine(int lineIndex)
{
    Line * line = Get(map.lines, lineIndex);
    Vertex * vertices = map.vertices->data;

    int x1 = (vertices[line->v1].origin.x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y1 = (vertices[line->v1].origin.y - mapBounds.y) / BLOCK_MAP_RESOLUTION;
    int x2 = (vertices[line->v2].origin.x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y2 = (vertices[line->v2].origin.y - mapBounds.y) / BLOCK_MAP_RESOLUTION;

    u8 frontFlags = 0;

    if ( x1 < x2 )
        frontFlags |= SOUTH_FRONT;

    if ( y1 < y2 )
        frontFlags |= WEST_FRONT;

    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int error = dx + dy;
    int error2;

    int x = x1;
    int y = y1;

    // The starting block is a vertex, flag as skip.
    BLOCK(x1, y1)->flags |= SKIP;

    while ( x != x2 || y != y2 )
    {
        Block * block = BLOCK((int)x, (int)y);
        block->lineIndex = lineIndex + 1;
        block->flags |= frontFlags;

#ifdef DRAW_BLOCK_MAP
        if ( block->flags & SKIP )
            SDL_SetRenderDrawColor(bmapRenderer, 255, 0, 255, 255);
        else
        {
            SDL_SetRenderDrawColor(bmapRenderer,
                                   0,
                                   (frontFlags & WEST_FRONT) ? 255 : 0,
                                   (frontFlags & SOUTH_FRONT) ? 255 : 0,
                                   255);
        }
        SDL_RenderDrawPoint(bmapRenderer, x, y);
#endif

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

    // The ending block is a vertex, flag as skip.
    BLOCK(x2, y2)->flags |= SKIP;

#ifdef DRAW_BLOCK_MAP
    DrawPixel(x2, y2, magenta);
#endif
}

#if DRAW_BLOCK_MAP
static void InitBlockMapWindow(void)
{
    if ( bmapWindow == NULL )
    {
        bmapWindow = SDL_CreateWindow("Sector Flood Fill Block Map",
                                      0,
                                      0,
                                      blockMapWidth * 2, blockMapHeight * 2,
                                      0);
        if ( bmapWindow == NULL )
            fprintf(stderr, "could not create bmap window: %s\n", SDL_GetError());

        u32 flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;
        bmapRenderer = SDL_CreateRenderer(bmapWindow, -1, flags);

        if ( bmapRenderer == NULL )
            fprintf(stderr, "could not create bmap renderer: %s\n", SDL_GetError());
    }
    else
    {
        SDL_SetWindowSize(bmapWindow, blockMapWidth * 2, blockMapHeight * 2);
    }

    if ( bmapTexture )
        SDL_DestroyTexture(bmapTexture);

    bmapTexture = SDL_CreateTexture(bmapRenderer,
                                    SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    blockMapWidth,
                                    blockMapHeight);

    if ( bmapTexture == NULL )
        fprintf(stderr, "could not create bmap texture: %s\n", SDL_GetError());
}
#endif

static void CreateBlockMap(void)
{
    mapBounds = GetMapBounds();

    blockMapWidth = mapBounds.w / BLOCK_MAP_RESOLUTION;
    blockMapHeight = mapBounds.h / BLOCK_MAP_RESOLUTION;
    printf("block map size: %d x %d\n", blockMapWidth, blockMapHeight);

    size_t newSize = blockMapWidth * blockMapHeight * sizeof(*blockMap);
    if ( newSize > allocated )
    {
        errno = 0;
        void * temp = realloc(blockMap, newSize);
        if ( temp == NULL ) {
            fprintf(stderr, "failed to allocate block map: %s\n", strerror(errno));
            return;
        }
        blockMap = temp;
        allocated = newSize;
        printf("allocated %zu bytes for block map\n", allocated);
    }

    memset(blockMap, 0, allocated);

#ifdef DRAW_BLOCK_MAP
    InitBlockMapWindow();

    // Render block map lines.
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
}

static void SelectLine(int index, int selectedSide)
{
    Line * lines = map.lines->data;
    index--;

    // Don't select the back of one-sided lines.
    if ( selectedSide == BACK_SELECTED && !(lines[index].flags & ML_TWOSIDED) )
        return;

    lines[index].selected = selectedSide;
}

bool cancelFill;

static void FloodFill(int x, int y, Direction direction)
{
    if ( x < 0 || x >= blockMapWidth || y < 0 || y >= blockMapHeight )
    {
        fprintf(stderr, "Fill point out of bounds! (%d, %d)\n", x, y);
        cancelFill = true;
        DeselectAllObjects();
        return;
    }

    Block * block = BLOCK(x, y);

    if ( cancelFill || block->flags & SKIP || block->flags & VISITED )
        return;

    if ( block->lineIndex && direction != NO_DIRECTION )
    {
        bool westFront = block->flags & WEST_FRONT;
        bool southFront = block->flags & SOUTH_FRONT;
        int index = block->lineIndex;

        if ( direction == EAST )
            SelectLine(index, westFront ? FRONT_SELECTED : BACK_SELECTED);
        else if ( direction == WEST )
            SelectLine(index, westFront ? BACK_SELECTED : FRONT_SELECTED);
        else if ( direction == NORTH )
            SelectLine(index, southFront ? FRONT_SELECTED : BACK_SELECTED);
        else if ( direction == SOUTH )
            SelectLine(index, southFront ? BACK_SELECTED : FRONT_SELECTED);

        return;
    }

    block->flags |= VISITED;

#ifdef DRAW_BLOCK_MAP
    DrawPixel(x, y, red);
    Refresh();
#endif

    FloodFill(x + 1, y, EAST);
    FloodFill(x - 1, y, WEST);
    FloodFill(x, y - 1, NORTH);
    FloodFill(x, y + 1, SOUTH);
}

void SelectSector(const SDL_Point * point)
{
    CreateBlockMap();
    DeselectAllObjects();

    // Convert to block map space and scale.
    int x = (point->x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y = (point->y - mapBounds.y) / BLOCK_MAP_RESOLUTION;

#ifdef DRAW_BLOCK_MAP
    SDL_SetRenderTarget(bmapRenderer, bmapTexture);
#endif

    cancelFill = false;
    FloodFill(x, y, NO_DIRECTION);
}
