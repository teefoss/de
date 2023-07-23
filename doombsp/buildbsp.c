#include "doombsp.h"
#include "limits.h"
#include <stdlib.h>

// (Comment in original)
// I assume that a grid 8 is used for the maps, so a point will be considered
// on a line if it is within 8 pixels of it.  The accounts for floating error.

int		cuts; // number of new lines generated by BSP process


void	DivlineFromWorldline (divline_t *d, line_t *w)
{
	d->pt = w->p1;
	d->dx = w->p2.x - w->p1.x;
	d->dy = w->p2.y - w->p1.y;
}

/// - Returns: side 0 (front), 1 (back), or -1 (colinear)
int	PointOnSide (NXPoint *p, divline_t *l)
{
	float	dx,dy;
	float	left, right;
	float	a,b,c,d;
	
	if (!l->dx)
	{
		if (p->x > l->pt.x-2 && p->x < l->pt.x+2)
			return -1;
		if (p->x < l->pt.x)
			return l->dy > 0;
		return l->dy < 0;
	}
	if (!l->dy)
	{
		if (p->y > l->pt.y-2 && p->y < l->pt.y+2)
			return -1;
		if (p->y < l->pt.y)
			return l->dx < 0;
		return l->dx > 0;
	}

	dx = l->pt.x - p->x;
	dy = l->pt.y - p->y;
	a = l->dx*l->dx + l->dy*l->dy;
	b = 2*(l->dx*dx + l->dy*dy);
	c = dx*dx+dy*dy - 2*2;		// 2 unit radius
	d = b*b - 4*a*c;
	if (d>0)
		return -1;		// within four pixels of line
	
	
	dx = p->x - l->pt.x;
	dy = p->y - l->pt.y;
	
	left = l->dy * dx;
	right = dy * l->dx;
	
	if ( fabs (left-right) < 0.5 )	// allow slop
		return -1;		// on line
	if (right < left)
		return 0;		// front side
	return 1;			// back side
}

int sign (float i)
{
	if (i<0)
		return -1;
	else if (i>0)
		return 1;
	return 0;
}

/// Determine whether line `wl` is on the front or back side of
/// dividing line `bl`.
///
/// If the line is colinear, it will be placed on the front side if
/// it is going the same direction as the dividing line.
///
/// - Returns: side 0 / 1, or -2 if line must be split.
int	LineOnSide (line_t *wl, divline_t *bl)
{
	int		s1,s2;
	float	dx, dy;
	
	s1 = PointOnSide (&wl->p1, bl);
	s2 = PointOnSide (&wl->p2, bl);

	if (s1 == s2)
	{
		if (s1 == -1)
		{	// colinear, so see if the directions are the same
			dx = wl->p2.x - wl->p1.x;
			dy = wl->p2.y - wl->p1.y;
			if (sign(dx) == sign (bl->dx) && sign(dy) == sign(bl->dy) )
				return 0;
			return 1;
		}
		return s1;
	}

	if (s1 == -1)
		return s2;
	if (s2 == -1)
		return s1;

	return -2;
}

/// - Returns: the fractional intercept point along first vector.
float InterceptVector (divline_t *v2, divline_t *v1)
{
	float	frac, num, den;
	
	den = v1->dy * v2->dx - v1->dx * v2->dy;

	if (den == 0)
		Error ("InterceptVector: parallel");

	num = (v1->pt.x - v2->pt.x)*v1->dy + (v2->pt.y - v1->pt.y)*v1->dx;
	frac = num / den;

	if (frac <= 0.0 || frac >= 1.0)
		Error ("InterceptVector: intersection outside line");
		
	return frac;
}

float NB_Round(float x)
{
	if (x>0)
	{
		if (x - (int)x < 0.1)
			return (int)x;
		else if (x - (int)x > 0.9)
			return (int)x+1;
		else
			return x;
	}
	
	if ((int)x - x < 0.1)
		return (int)x;
	else if ((int)x - x > 0.9)
		return  (int)x - 1;
	return x;
}

/// Truncates the given worldline to the front side of the divline.
///
/// - Returns: the cut off back side in a newly allocated worldline.
line_t * CutLine(line_t * wl, divline_t * bl)
{
	int			side;
	line_t		*new_p;
	divline_t	wld;
	float		frac;
	NXPoint		intr;
	int			offset;
	
	cuts++;
	DivlineFromWorldline (&wld, wl);
	new_p = malloc (sizeof(line_t));
	memset (new_p,0,sizeof(*new_p));
	*new_p = *wl; 
	
	frac = InterceptVector (&wld, bl);
	intr.x = wld.pt.x + NB_Round(wld.dx*frac);
	intr.y = wld.pt.y + NB_Round(wld.dy*frac);
	offset = wl->offset + NB_Round(frac*sqrt(wld.dx*wld.dx+wld.dy*wld.dy));
	side = PointOnSide (&wl->p1, bl);
	if (side == 0)
	{	// line starts on front side
		wl->p2 = intr;
		new_p->p1 = intr;
		new_p->offset = offset;
	}
	else
	{	// line starts on back side
		wl->p1 = intr;
		wl->offset = offset;
		new_p->p2 = intr;
	}
	
	return new_p;
}


/// Returns a number grading the quality of a split along the givent line
/// for the current list of lines.
///
/// Evaluation is halted as soon as it is
/// determined that a better split already exists.
/// A split is good if it divides the lines evenly without cutting many lines.
/// A horizontal or vertical split is better than a sloping split.
/// The LOWER the returned value, the better.  If the split line does not divide
/// any of the lines at all, `INT_MAX` will be returned.
int EvaluateSplit(Array * lines_i, line_t * spliton, int bestgrade)
{
	int				i,c,side;
	line_t			*line_p;
	divline_t		divline;
	int				frontcount, backcount, max, new;
	int				grade;

	DivlineFromWorldline(&divline, spliton);
	
	frontcount = backcount = 0;
	c = lines_i->count;
	grade = 0;
		
	for ( i = 0; i < c; i++ )
	{
		line_p = Get(lines_i, i);
		if (line_p == spliton)
			side = 0;
		else
			side = LineOnSide (line_p, &divline);
		switch (side)
		{
		case 0:
			frontcount++;
			break;
		case 1:
			backcount++;
			break;
		case -2:
			frontcount++;
			backcount++;
			break;
		}
		
		max = MAX(frontcount,backcount);
		new = (frontcount+backcount) - c;
		grade = max+new*8;
		if (grade > bestgrade)
			return grade;		// might as well stop now
	}
	
	if (frontcount == 0 || backcount == 0)
		return INT_MAX;			// line does not partition at all
		
	return grade;
}

/// Actually splits the line list as `EvaluateLines` predicted.
void ExecuteSplit(Array * lines_i,
                  line_t * spliton,
                  Array * frontlist_i,
                  Array * backlist_i)
{
	int				i,c,side;
	line_t			*line_p, *newline_p;
	divline_t		divline;
	
	DivlineFromWorldline (&divline, spliton);
	DrawDivLine (&divline);
	
	c = lines_i->count;
		
	for (i=0 ; i<c ; i++)
	{
		line_p = Get(lines_i, i);

		if (line_p == spliton)
			side = 0;
		else
			side = LineOnSide (line_p, &divline);
        
		switch (side)
        {
            case 0:
                Push(frontlist_i, line_p);
                break;
            case 1:
                Push(backlist_i, line_p);
                break;
            case -2:
                newline_p = CutLine (line_p, &divline);
                Push(frontlist_i, line_p);
                Push(backlist_i, newline_p);
                break;
            default:
                Error ("ExecuteSplit: bad side");
                break;
        }
	}
}


static float gray = 1.0f;

/// Takes a storage of lines and recursively partitions the list.
///
/// - Returns: a `bspnode_t`.
bspnode_t * BSPList(Array * lines_i)
{
	Array 		    *frontlist_i, *backlist_i;
	int				c, step;
	line_t			*line_p, *bestline_p;
	int				v, bestv;
	bspnode_t		*node_p;
	
	if ( draw )
    {
        SDL_SetRenderDrawColor(nbRenderer,
                               gray * 255,
                               gray * 255,
                               gray * 255,
                               255);

        gray = 1.0f - gray;
        SDL_SetRenderTarget(nbRenderer, nbTexture);
    }

    DrawLineStore (lines_i);

	node_p = malloc (sizeof(*node_p));
	memset (node_p, 0, sizeof(*node_p));

    //
    // find the best line to partition on
    //
	c = lines_i->count;
	bestv = INT_MAX;
	bestline_p = NULL;
	step = (c/40)+1;		// set this to 1 for an exhaustive search

research:
	for ( int i = 0; i < c; i += step )
	{
		line_p = Get(lines_i, i);
		v = EvaluateSplit (lines_i, line_p, bestv);
		if (v<bestv)
		{
			bestv = v;
			bestline_p = line_p;
		}
	}
	
    //
    // If none of the lines should be split, the remaining lines
    // are convex, and form a terminal node.
    //
    // printf ("bestv:%i\n",bestv);

	if (bestv == INT_MAX)
	{
		if (step > 1)
		{	// possible to get here with non convex area if BSPSLIDE specials
			// caused rejections
			step = 1;
			goto research;
		}
		node_p->lines_i = lines_i;
		return node_p;
	}
	
    //
    // divide the line list into two nodes along the best split line
    //
	DivlineFromWorldline (&node_p->divline, bestline_p);

    frontlist_i = NewArray(0, sizeof(line_t), 1);
    backlist_i = NewArray(0, sizeof(line_t), 1);

	ExecuteSplit (lines_i, bestline_p, frontlist_i, backlist_i);

    //
    // recursively divide the lists
    //
	node_p->side[0] = BSPList(frontlist_i);
	node_p->side[1] = BSPList(backlist_i);
	
	return node_p;
}


Array * segstore_i;

void MakeSegs(void)
{
    segstore_i = NewArray(0, sizeof(line_t), 1);
	
	int count = linestore_i->count;

	for ( int i = 0; i < count; i++ )
	{
        Line * wl = Get(linestore_i, i);

        line_t li;
		li.p1 = wl->p1;
		li.p2 = wl->p2;
		li.linedef = i;
		li.side = 0;
		li.offset = 0;
		li.grouped = false;

        Push(segstore_i, &li);
		
		if (wl->flags & ML_TWOSIDED)
		{
			li.p1 = wl->p2;
			li.p2 = wl->p1;
			li.linedef = i;
			li.side = 1;
			li.offset = 0;
			li.grouped = false;

            Push(segstore_i, &li);
		}
	}
}


bspnode_t * startnode;

void BuildBSP(void)
{
	MakeSegs();
	cuts = 0;

    if ( draw )
        puts("BSPList");

	startnode = BSPList(segstore_i);
}
