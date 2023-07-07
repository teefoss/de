// doomload.m
#include "doombsp.h"
#include "map.h"
#include <limits.h>

Array    *linestore_i, *thingstore_i;
int		linenum = 0;

/*
=================
=
= ReadLine
=
=================
*/

#if 0
worldline_t *ReadLine (FILE *file)
{
	worldline_t	*line;
	NXPoint		*p1, *p2;
	worldside_t	*s;
	sectordef_t	*e;
	int			i;
	
	line = malloc(sizeof(*line));
	memset (line, 0, sizeof(*line));

	p1 = &line->p1;
	p2 = &line->p2;
	
	if (fscanf (file,"(%f,%f) to (%f,%f) : %d : %d : %d\n"
		,&p1->x, &p1->y,&p2->x, &p2->y,&line->flags
		, &line->special, &line->tag) != 7)
		Error ("Failed ReadLine");
	
	for (i=0 ; i<=  ( (line->flags&ML_TWOSIDED) != 0) ; i++)
	{
		s = &line->side[i];	
		if (fscanf (file,"    %d (%d : %s / %s / %s )\n"
			,&s->firstrow, &s->firstcollumn, s->toptexture, s->bottomtexture, s->midtexture) != 5)
			Error ("Failed ReadLine (side)");
		e = &s->sectordef;
		if (fscanf (file,"    %d : %s %d : %s %d %d %d\n"
			,&e->floorheight, e->floorflat, &e->ceilingheight
			,e->ceilingflat,&e->lightlevel, &e->special, &e->tag) != 7)
			Error ("Failed ReadLine (sector)");
		if (!strcmp (e->floorflat,"-"))
			printf ("WARNING: line %i has no sectordef\n",linenum);
	}
	
	linenum++;
	
	return line;
}

/*
=================
=
= ReadThing
=
=================
*/

worldthing_t *ReadThing (FILE *file)
{
	int				x,y;
	worldthing_t	*thing;
	
	thing = malloc(sizeof(*thing));
	memset (thing, 0, sizeof(*thing));

	if (fscanf (file,"(%i,%i, %d) :%d, %d\n"
		,&x, &y, &thing->angle, &thing->type, &thing->options) != 5)
		Error ("Failed ReadThing");

	thing->origin.x = x & -16;
	thing->origin.y = y & -16;
	
	return thing;
}

#endif

/*
==================
=
= LineOverlaid
=
= Check to see if the line is colinear and overlapping any previous lines
==================
*/

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

boolean LineOverlaid(Line * line)
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

#if 0
/*
===================
=
= LoadDoomMap
=
===================
*/

/// Load map from .dwd, populating doombsp arrays linestore_i and thingstore_i

void LoadDoomMap (char *mapname)
{
	FILE 		*file;
	int			i, version;
	int			linecount, thingcount;
	worldline_t	*line;
	
	file = fopen (mapname,"r");
	if (!file)
		Error ("LoadDoomMap: couldn't open %s", mapname);
	
	if (!fscanf (file, "WorldServer version %d\n", &version) || version != 4)
		Error ("LoadDoomMap: not a version 4 doom map");
	printf ( "Loading version 4 doom map: %s\n",mapname);
		
//
// read lines
//	
	if (fscanf (file,"\nlines:%d\n",&linecount) != 1)
		Error ("LoadDoomMap: can't read linecount");
	printf ("%i lines\n", linecount);
//	linestore_i = [[Storage alloc]
//		initCount:		0
//		elementSize:	sizeof(worldline_t)
//		description:	NULL];
    linestore_i = NewArray(0, sizeof(worldline_t), 1);

	for (i=0 ; i<linecount ; i++)
	{
		line = ReadLine (file);
		if (line->p1.x == line->p2.x && line->p1.y == line->p2.y)
		{
			printf ("WARNING: line %i is length 0 (removed)\n",i);
			continue;
		}
		if (LineOverlaid (line))
		{
			printf ("WARNING: line %i is overlaid (removed)\n",i);
			continue;
		}
//		[linestore_i addElement: line];
        Push(linestore_i, line);
	}
		
//
// read things
//
	if (fscanf (file,"\nthings:%d\n",&thingcount) != 1)
		Error ("LoadDoomMap: can't read thingcount");
	printf ( "%i things\n", thingcount);
//	thingstore_i = [[Storage alloc]
//		initCount:		0
//		elementSize:	sizeof(worldthing_t)
//		description:	NULL];
    thingstore_i = NewArray(0, sizeof(worldthing_t), 1);
		
	for (i=0 ; i<thingcount ; i++)
//		[thingstore_i addElement: ReadThing (file)];
        Push(thingstore_i, ReadThing(file));

	fclose (file);

}

#endif

/// Populate node builder arrays linestore_i and thingstore_i, skipping any
/// invalid data.
void NodeBuilderLoadMap(void)
{
    linestore_i = NewArray(map.lines->count, sizeof(Line), 0);

    Vertex * vertices = map.vertices->data;
    Line * lines = map.lines->data;
    Line * line = lines;

    for ( int i = 0; i < map.lines->count; i++, line++ )
    {
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
        Thing rounded = *thing;
        rounded.origin.x &= -16;
        rounded.origin.y &= -16;
        rounded.origin.y = -rounded.origin.y;

        Push(thingstore_i, &rounded);
    }

    printf("Lines: %d; Things: %d\n", linestore_i->count, thingstore_i->count);
}
