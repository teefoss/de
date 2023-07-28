//
//  sector.c
//  de
//
//  Created by Thomas Foster on 6/21/23.
//

#include "e_sector.h"
#include "m_map.h"
#include "doomdata.h"
#include <errno.h>

#define BLOCK_MAP_RESOLUTION 8 // How much the block map is scaled down by.
#define VISITED 0x8000 // Visited during flood fill.

// Flood Fill Direction
typedef enum {
    NO_DIRECTION, // Just used for the initial click point.
    NORTH,
    EAST,
    SOUTH,
    WEST
} Direction;

// Highly tweaked and simplified version of DoomEd's sector flood fill.
// --------------------------------------------------------------------
// - All lines (their indices) are drawn to the block map (Bresenham).
// - The problem of multiple lines sharing a single vertex is solved by
//   simply skipping them.
// - To determine which side to select, track the direction of travel during
//   flood fill then simply compare points' coordinates:
//      x1 < x2 ? The south-facing side is the front
//      y1 < y2 ? The west-facing side is the front

static SDL_Rect mapBounds;
static u16 * blockMap;      // Each block is the index of the line.
static size_t allocated;    // Track blockMap size so it can be reallocated.
static int blockMapWidth;
static int blockMapHeight;



#ifdef DRAW_BLOCK_MAP
SDL_Window * bmapWindow;
SDL_Renderer * bmapRenderer;
SDL_Texture * bmapTexture;

SDL_Rect bmapLocation;
float bmapScale = 1.0f;

static SDL_Color red = { 255, 0, 0, 255 };
static SDL_Color magenta = { 255, 0, 255, 255 };

// Assume target is set to bmapTexture!
static void DrawPixel(int x, int y, SDL_Color c)
{
    SDL_SetRenderDrawColor(bmapRenderer, c.r, c.g, c.b, c.a);
    SDL_RenderDrawPoint(bmapRenderer, x, y);
}

// Force immediate render of the block map texture
static void RefreshBlockmap(void)
{
    SDL_Rect dst = bmapLocation;
    int w, h;
    SDL_QueryTexture(bmapTexture, NULL, NULL, &w, &h);
    dst.w = w * bmapScale;
    dst.h = h * bmapScale;

    Refresh(bmapRenderer, bmapTexture, &dst);
}
#endif


static void DrawBlockMapLine(int lineIndex)
{
    Line * line = Get(map.lines, lineIndex);
    Vertex * vertices = map.vertices->data;

    int x1 = (vertices[line->v1].origin.x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y1 = (vertices[line->v1].origin.y - mapBounds.y) / BLOCK_MAP_RESOLUTION;
    int x2 = (vertices[line->v2].origin.x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y2 = (vertices[line->v2].origin.y - mapBounds.y) / BLOCK_MAP_RESOLUTION;

    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int error = dx + dy;
    int error2;

    int x = x1;
    int y = y1;

    while ( x != x2 || y != y2 )
    {
        // Add one to the index so that blocks with '0' can indicate
        // no line is present.
        blockMap[y * blockMapWidth + x] = lineIndex + 1;

#ifdef DRAW_BLOCK_MAP
        SDL_SetRenderDrawColor(bmapRenderer,
                               0,
                               y1 < y2 ? 255 : 0, // west-facing front
                               x1 < x2 ? 255 : 0, // south-facing front
                               255);
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

    // Skip all vertices.
    blockMap[y1 * blockMapWidth + x1] |= VISITED;
    blockMap[y2 * blockMapWidth + x2] |= VISITED;

#ifdef DRAW_BLOCK_MAP
    DrawPixel(x1, y1, magenta);
    DrawPixel(x2, y2, magenta);
#endif
}

#ifdef DRAW_BLOCK_MAP
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
        if ( !line->deleted )
            DrawBlockMapLine(i);
    }
}

// Early exit, if trying to fill outside map bounds (someone tried to select
// outside of all sectors).
bool cancelFill;

static void FloodFill(int x, int y, Direction direction)
{
    if ( cancelFill )
        return;

    if ( x < 0 || x >= blockMapWidth || y < 0 || y >= blockMapHeight )
    {
        fprintf(stderr, "Fill point out of bounds! (%d, %d)\n", x, y);
        cancelFill = true;
        DeselectAllObjects();
        return;
    }

    u16 * block = &blockMap[y * blockMapWidth + x];
    if ( *block & VISITED )
        return;

    int index = *block & ~VISITED;

    if ( index && direction != NO_DIRECTION )
    {
        SDL_Point p1, p2;
        GetLinePoints(index - 1, &p1, &p2);
        bool westFront = p1.y < p2.y;
        bool southFront = p1.x < p2.x;

        if ( direction == EAST )
            SelectLine(index - 1, westFront ? SIDE_FRONT : SIDE_BACK);
        else if ( direction == WEST )
            SelectLine(index - 1, westFront ? SIDE_BACK : SIDE_FRONT);
        else if ( direction == NORTH )
            SelectLine(index - 1, southFront ? SIDE_FRONT : SIDE_BACK);
        else if ( direction == SOUTH )
            SelectLine(index - 1, southFront ? SIDE_BACK : SIDE_FRONT);

        return;
    }

    *block |= VISITED;

#ifdef DRAW_BLOCK_MAP
    DrawPixel(x, y, red);
    RefreshBlockmap();
#endif

    FloodFill(x + 1, y, EAST);
    FloodFill(x - 1, y, WEST);
    FloodFill(x, y - 1, NORTH);
    FloodFill(x, y + 1, SOUTH);
}

// Returns true if at least one line was selected.
bool SelectSector(const SDL_Point * point)
{
    CreateBlockMap();
    DeselectAllObjects();

    // Convert to block map space and scale.
    int x = (point->x - mapBounds.x) / BLOCK_MAP_RESOLUTION;
    int y = (point->y - mapBounds.y) / BLOCK_MAP_RESOLUTION;

    cancelFill = false;
    FloodFill(x, y, NO_DIRECTION);

    return editor.numSelectedLines > 0;
}


