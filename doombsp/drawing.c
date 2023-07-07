#include "doombsp.h"

SDL_Window * nbWindow;
SDL_Renderer * nbRenderer;
SDL_Texture * nbTexture; // TODO: factor so these can be static

static float	scale = 0.25;
NXRect		    worldbounds;


/// Makes the rectangle just touch the two points
void IDRectFromPoints(NXRect *rect, NXPoint const *p1, NXPoint const *p2 )
{
    // return a rectangle that encloses the two points
	if (p1->x < p2->x)
	{
		rect->origin.x = p1->x;
		rect->size.width = p2->x - p1->x + 1;
	}
	else
	{
		rect->origin.x = p2->x;
		rect->size.width = p1->x - p2->x + 1;
	}
	
	if (p1->y < p2->y)
	{
		rect->origin.y = p1->y;
		rect->size.height = p2->y - p1->y + 1;
	}
	else
	{
		rect->origin.y = p2->y;
		rect->size.height = p1->y - p2->y + 1;
	}
}

/// Make the rect enclose the point if it doesn't allready
void IDEnclosePoint (NXRect *rect, NXPoint const *point)
{
	float	right, top;
	
	right = rect->origin.x + rect->size.width - 1;
	top = rect->origin.y + rect->size.height - 1;
	
	if (point->x < rect->origin.x)
		rect->origin.x = point->x;
	if (point->y < rect->origin.y)
		rect->origin.y = point->y;		
	if (point->x > right)
		right = point->x;
	if (point->y > top)
		top = point->y;
		
	rect->size.width = right - rect->origin.x + 1;
	rect->size.height = top - rect->origin.y + 1;
}

void NB_Refresh(int delayMS)
{
    Refresh(nbRenderer, nbTexture);
    SDL_Delay(delayMS);
}

/// Find the world bounds given lines in array. Result in `r`.
void BoundLineStore(Array * lines_i, NXRect * r)
{
	int				i, c;
	Line		    *line_p;
	
	c = lines_i->count;
	if (!c)
		Error ("BoundLineStore: empty list");
		
	line_p = Get(lines_i, 0);
	IDRectFromPoints (r, &line_p->p1, &line_p->p2);
	
	for ( i = 1; i < c; i++ )
	{
		line_p = Get(lines_i, i);
		IDEnclosePoint(r, &line_p->p1);
		IDEnclosePoint(r, &line_p->p2);
	}	
}

/// Draw line in the node builder window, accounting for the world origin
/// and scale. Assumes target is set to `nbTexture`
void NB_DrawLine(float x1, float y1, float x2, float y2)
{
    SDL_RenderDrawLineF(nbRenderer,
                        (x1 - worldbounds.origin.x) * scale,
                        (y1 - worldbounds.origin.y) * scale,
                        (x2 - worldbounds.origin.x) * scale,
                        (y2 - worldbounds.origin.y) * scale);
}

void DrawLineStore(Array * lines_i)
{
	int				i,c;
	Line		    *line_p;
	
	if ( !draw )
		return;
		
	c = lines_i->count;

	for ( i = 0; i < c; i++ )
	{
		line_p = Get(lines_i, i);
        NB_DrawLine(line_p->p1.x, line_p->p1.y, line_p->p2.x, line_p->p2.y);
	}

    NB_Refresh(0);
}

/// Draws all of the lines in the given storage object
void DrawLineDef (maplinedef_t *ld)
{
	mapvertex_t		*v1, *v2;
	
	if (!draw)
		return;
	
	v1 = Get(mapvertexstore_i, ld->v1);
	v2 = Get(mapvertexstore_i, ld->v2);

    NB_DrawLine(v1->x, v1->y, v2->x, v2->y);
    NB_Refresh(0);
}

void NB_DrawMap (void)
{
	NXRect	scaled;
	
	BoundLineStore(linestore_i, &worldbounds);
	worldbounds.origin.x -= 8;
	worldbounds.origin.y -= 8;
	worldbounds.size.width += 16;
	worldbounds.size.height += 16;
	
	if (!draw)
		return;
		
	scaled.origin.x = 300;
	scaled.origin.y = 80;
	scaled.size.width = worldbounds.size.width*scale;
	scaled.size.height = worldbounds.size.height* scale;

    nbWindow = SDL_CreateWindow("",
                                0,
                                0,
                                scaled.size.width,
                                scaled.size.height,
                                0);

    if ( nbWindow == NULL )
        puts("nbwindow NULL!");

    int flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;
    nbRenderer = SDL_CreateRenderer(nbWindow, -1, flags);

    if ( nbRenderer == NULL )
        puts("nbrenderer NULL!");

    nbTexture = SDL_CreateTexture(nbRenderer,
                                  SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_TARGET,
                                  scaled.size.width,
                                  scaled.size.height);

    if ( nbTexture == NULL )
        puts("nbtexture NULL1");

    SDL_PumpEvents();

    SDL_SetRenderTarget(nbRenderer, nbTexture);
    SDL_SetRenderDrawColor(nbRenderer, 192, 192, 192, 255);
    SDL_RenderClear(nbRenderer);
    SDL_SetRenderDrawColor(nbRenderer, 0, 0, 0, 255);

	DrawLineStore(linestore_i);
}

void EraseWindow (void)
{
	if (!draw)
		return;

    u8 r, g, b, a;
    SDL_GetRenderDrawColor(nbRenderer, &r, &g, &b, &a);

    SDL_SetRenderDrawColor(nbRenderer, 160, 160, 160, 255);
    SDL_RenderClear(nbRenderer);

    SDL_SetRenderDrawColor(nbRenderer, r, g, b, a);
}

void DrawDivLine (divline_t *div)
{
	float	vx,vy, dist;
	
	if (!draw)
		return;
		
    SDL_SetRenderDrawColor(nbRenderer, 255, 0, 0, 255);
	
	dist = sqrt (pow(div->dx,2)+pow(div->dy,2));
	vx = div->dx/dist;
	vy = div->dy/dist;
	
	dist = MAX(worldbounds.size.width,worldbounds.size.height);

    NB_DrawLine(div->pt.x - vx * dist, div->pt.y - vy * dist,
                div->pt.x + vx * dist, div->pt.y + vy * dist);

    NB_Refresh(0);
}

