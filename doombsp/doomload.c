// doomload.m
#include "doombsp.h"
#include "m_map.h"
#include <limits.h>

// TF: Stores map.lines and map.things, filtered, and coordinates converted
// before building.
Array * linestore_i;
Array * thingstore_i;


typedef struct
{
	float	left, right, top, bottom;
} bbox_t;

void BBoxFromPoints (bbox_t *box, NXPoint *p1, NXPoint *p2)
{
	if (p1->x < p2->x)
	{
		box->left = p1->x;
		box->right = p2->x;
	}
	else
	{
		box->left = p2->x;
		box->right = p1->x;
	}
	if (p1->y < p2->y)
	{
		box->bottom = p1->y;
		box->top = p2->y;
	}
	else
	{
		box->bottom = p2->y;
		box->top = p1->y;
	}
}

/// Check to see if the line is colinear and overlapping any previous lines.
bool LineOverlaid(Line * line)
{
	int		    j, count;
	Line	    *scan;
	divline_t	wl;
	bbox_t		linebox, scanbox;
	
	wl.pt = line->p1;
	wl.dx = line->p2.x - line->p1.x;
	wl.dy = line->p2.y - line->p1.y;

	count = linestore_i->count;
    if ( count == 0 )
        return false;

	scan = Get(linestore_i, 0);
	for (j=0 ; j<count ; j++, scan++)
	{
		if (PointOnSide (&scan->p1, &wl) != -1)
			continue;
		if (PointOnSide (&scan->p2, &wl) != -1)
			continue;

        // line is colinear, see if it overlapps
		BBoxFromPoints (&linebox, &line->p1, &line->p2);
		BBoxFromPoints (&scanbox, &scan->p1, &scan->p2);
				
		if (linebox.right  > scanbox.left && linebox.left < scanbox.right)
			return true;
		if (linebox.bottom < scanbox.top && linebox.top > scanbox.bottom)
			return true;
	}

	return false;
}

void NB_LoadMap(void)
{
    linestore_i = NewArray(map.lines->count, sizeof(Line), 0);

    Vertex * vertices = map.vertices->data;
    Line * lines = map.lines->data;
    Line * line = lines;

    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
        if ( line->deleted )
            continue;
        line->p1.x =  vertices[line->v1].origin.x;
        line->p1.y = -vertices[line->v1].origin.y; // SDL to NeXT
        line->p2.x =  vertices[line->v2].origin.x;
        line->p2.y = -vertices[line->v2].origin.y; // SDL to NeXT

        if ( line->p1.x == line->p2.x && line->p1.y == line->p2.y )
        {
            printf("Warning: Line %d has length of 0 and will be skipped\n", i);
            continue;
        }

        if ( LineOverlaid(line) )
        {
            printf("Warning: Line %d if overlaid with another and will"
                   "be skipped.\n", i);
            continue;
        }

        Push(linestore_i, line);
    }

    thingstore_i = NewArray(map.things->count, sizeof(Thing), 0);

    Thing * things = map.things->data;
    Thing * thing = things;

    for ( int i = 0; i < map.things->count; i++, thing++ )
    {
        if ( thing->deleted )
            continue;
        
        Thing rounded = *thing;
        rounded.origin.x &= -16;
        rounded.origin.y &= -16;
        rounded.origin.y = -rounded.origin.y;

        Push(thingstore_i, &rounded);
    }

    printf("Lines: %d; Things: %d\n", linestore_i->count, thingstore_i->count);
}
