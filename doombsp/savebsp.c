// savebsp.m

#include "doombsp.h"
#include "m_thing.h"
#include "e_editor.h"
#include "m_bbox.h"
#include <limits.h>

Array * secstore_i; // [] of mapsector_t
Array * mapvertexstore_i;
Array * subsecstore_i;
Array * maplinestore_i; // [] of mapseg_t!
Array * nodestore_i;
Array * mapthingstore_i;
Array * ldefstore_i;
Array * sdefstore_i; // [] of mapsidedef_t!


void WriteStorage(char * name, Array * store, int esize)
{
	int count = store->count;
	int len = esize * count;

    AddLump(editor.pwad, name, store->data, len);
	printf("%s (%i): %i\n", name, count, len);
}

void OutputSectors (void)
{
	int		i, count;
	mapsector_t		*p;

	count = secstore_i->count;
	p = Get(secstore_i, 0);
	for (i=0 ; i<count ; i++, p++)
	{
		p->floorheight = SWAP16(p->floorheight);
		p->ceilingheight = SWAP16(p->ceilingheight);
		p->lightlevel = SWAP16(p->lightlevel);
		p->special = SWAP16(p->special);
		p->tag = SWAP16(p->tag);
	}	
	WriteStorage ("sectors", secstore_i, sizeof(mapsector_t));
}

void OutputSegs (void)
{
	int		i, count;
	mapseg_t		*p;

	count = maplinestore_i->count;
	p = Get(maplinestore_i, 0);

	for ( i = 0; i < count; i++, p++ )
	{
		p->v1 = SWAP16(p->v1);
		p->v2 = SWAP16(p->v2);
		p->angle = SWAP16(p->angle);
		p->linedef = SWAP16(p->linedef);
		p->side = SWAP16(p->side);
		p->offset = SWAP16(p->offset);
	}	
	WriteStorage ("segs",maplinestore_i, sizeof(mapseg_t));
}

void OutputSubsectors (void)
{
	int		i, count;
	mapsubsector_t		*p;

	count = subsecstore_i->count;
	p = Get(subsecstore_i, 0);
	for (i=0 ; i<count ; i++, p++)
	{
		p->numsegs = SWAP16(p->numsegs);
		p->firstseg = SWAP16(p->firstseg);
	}	
	WriteStorage ("ssectors", subsecstore_i, sizeof(mapsubsector_t));
}

void OutputVertexes (void)
{
	int		i, count;
	mapvertex_t		*p;

	count = mapvertexstore_i->count;
	p = Get(mapvertexstore_i, 0);
	for (i=0 ; i<count ; i++, p++)
	{
		p->x = SWAP16(p->x);
		p->y = SWAP16(p->y);
	}	
	WriteStorage ("vertexes",mapvertexstore_i, sizeof(mapvertex_t));
}

void OutputThings(void)
{
	int		i, count;
	mapthing_t		*p;

	count = mapthingstore_i->count;
	p = Get(mapthingstore_i, 0);
	for ( i = 0; i < count; i++, p++ )
	{
		p->x = SWAP16(p->x);
		p->y = SWAP16(p->y);
		p->angle = SWAP16(p->angle);
		p->type = SWAP16(p->type);
		p->options = SWAP16(p->options);
	}

	WriteStorage ("things", mapthingstore_i, sizeof(mapthing_t));
}

void OutputLineDefs (void)
{
	int		i, count;
	maplinedef_t		*p;

	count = ldefstore_i->count;
	p = Get(ldefstore_i, 0);
	for (i=0 ; i<count ; i++, p++)
	{
		p->v1 = SWAP16(p->v1);
		p->v2 = SWAP16(p->v2);
        // some ancient version of DoomEd left ML_MAPPED flags in
        // some of the levels
		p->flags = SWAP16(p->flags&~ML_MAPPED);
		p->special = SWAP16(p->special);
		p->tag = SWAP16(p->tag);
		p->sidenum[0] = SWAP16(p->sidenum[0]);
		p->sidenum[1] = SWAP16(p->sidenum[1]);
	}
    
	WriteStorage ("linedefs", ldefstore_i, sizeof(maplinedef_t));
}

void OutputSideDefs (void)
{
	int		i, count;
	mapsidedef_t		*p;

	count = sdefstore_i->count;
	p = Get(sdefstore_i, 0);
	for (i=0 ; i<count ; i++, p++)
	{
		p->textureoffset = SWAP16(p->textureoffset);
		p->rowoffset = SWAP16(p->rowoffset);
		p->sector = SWAP16(p->sector);
	}

	WriteStorage ("sidedefs", sdefstore_i, sizeof(mapsidedef_t));
}

void OutputNodes (void)
{
	int		i, count;
	mapnode_t		*p;

	count = nodestore_i->count;
	p = Get(nodestore_i, 0);

	for (i=0 ; i<count ; i++, p++)
	{
		for ( size_t j = 0; j < sizeof(mapnode_t) / 2; j++ )
        {
            // TODO: unroll this.
            ((short *)p)[j] = SWAP16(((short *)p)[j]);
        }
	}

	WriteStorage ("nodes", nodestore_i, sizeof(mapnode_t));
}


// -----------------------------------------------------------------------------
// Processing

/// Returns the vertex number, adding a new vertex if needed.
int UniqueVertex (int x, int y)
{
	int				i, count;
	mapvertex_t		mv, *mvp;
	
	mv.x = x;
	mv.y = y;
	
    // see if an identical vertex already exists
	count = mapvertexstore_i->count;
	mvp = Get(mapvertexstore_i, 0);
	for (i=0 ; i<count ; i++, mvp++)
    {
        if (mvp->x == mv.x && mvp->y == mv.y)
            return i;
    }

    Push(mapvertexstore_i, &mv);
	
	return count;	
}


float	bbox[4];

void AddPointToBBox (NXPoint *pt)
{
	if (pt->x < bbox[BOXLEFT])
		bbox[BOXLEFT] = pt->x;
	if (pt->x > bbox[BOXRIGHT])
		bbox[BOXRIGHT] = pt->x;
		
	if (pt->y > bbox[BOXTOP])
		bbox[BOXTOP] = pt->y;
	if (pt->y < bbox[BOXBOTTOM])
		bbox[BOXBOTTOM] = pt->y;
}

///Adds the lines in a subsector to the mapline storage
void ProcessLines (Array *store_i)
{
	int			i,count;
	line_t 		*wline;
	mapseg_t	line;
	short		angle;
	float		fangle;
	
	bbox[BOXLEFT] = INT_MAX;
	bbox[BOXRIGHT] = INT_MIN;
	bbox[BOXTOP] = INT_MIN;
	bbox[BOXBOTTOM] = INT_MAX;
	
	count = store_i->count;
	for (i=0 ; i<count ; i++)
	{
		wline = Get(store_i, i);
		if (wline->grouped)
			printf ("ERROR: line regrouped\n");
		wline->grouped = true;
		
		memset (&line, 0, sizeof(line));
		AddPointToBBox (&wline->p1);
		AddPointToBBox (&wline->p2);
		line.v1 = UniqueVertex (wline->p1.x, wline->p1.y);
		line.v2 = UniqueVertex (wline->p2.x, wline->p2.y);
		line.linedef = wline->linedef;
		line.side = wline->side;
		line.offset = wline->offset;
		fangle = atan2 (wline->p2.y - wline->p1.y, wline->p2.x - wline->p1.x);
		angle = (short)(fangle/(PI*2)*0x10000);
		line.angle = angle;

        Push(maplinestore_i, &line);
	}
}

int ProcessSubsector(Array * wmaplinestore_i) // A node's lines (line_t[])
{
	int				count;
	mapsubsector_t	sub;
	
	memset (&sub,0,sizeof(sub));
	
	count = wmaplinestore_i->count; // The number of this node's lines.
	if (count < 1)
		Error ("ProcessSubsector: count = %i",count);

	sub.numsegs = count;
	sub.firstseg = maplinestore_i->count;
	ProcessLines (wmaplinestore_i);
	
    // add the new subsector
    Push(subsecstore_i, &sub);
	
	return subsecstore_i->count-1;
}

int ProcessNode (bspnode_t *node, short *totalbox)
{
	short		subbox[2][4];
	int			i, r;
	mapnode_t	mnode;
	
	memset (&mnode, 0, sizeof(mnode));

	if ( node->lines_i )	// NF_SUBSECTOR flags a subsector
	{
		r = ProcessSubsector (node->lines_i);
		for (i=0 ; i<4 ; i++)
			totalbox[i] = bbox[i];
		return r | NF_SUBSECTOR;
	}
	
	mnode.x = node->divline.pt.x;
	mnode.y = node->divline.pt.y;
	mnode.dx = node->divline.dx;
	mnode.dy = node->divline.dy;
	
	r = ProcessNode(node->side[0], subbox[0]);
	mnode.children[0] = r;
	for (i=0 ; i<4 ; i++)
		mnode.bbox[0][i] = subbox[0][i];
	
	r = ProcessNode (node->side[1], subbox[1]);
	mnode.children[1] =r;
	for (i=0 ; i<4 ; i++)
		mnode.bbox[1][i] = subbox[1][i];

	totalbox[BOXLEFT] = MIN(subbox[0][BOXLEFT], subbox[1][BOXLEFT]);
	totalbox[BOXTOP] = MAX(subbox[0][BOXTOP], subbox[1][BOXTOP]);
	totalbox[BOXRIGHT] = MAX(subbox[0][BOXRIGHT], subbox[1][BOXRIGHT]);
	totalbox[BOXBOTTOM] = MIN(subbox[0][BOXBOTTOM], subbox[1][BOXBOTTOM]);

    Push(nodestore_i, &mnode);
	return nodestore_i->count - 1;
}

/// Recursively builds the nodes, subsectors, and line lists,
/// then writes the lumps.
void ProcessNodes (void)
{
	short	worldbounds_[4];

    subsecstore_i = NewArray(0, sizeof(mapsubsector_t), 1);
    maplinestore_i = NewArray(0, sizeof(mapseg_t), 1);
    nodestore_i = NewArray(0, sizeof(mapnode_t), 1);

	ProcessNode (startnode, worldbounds_);
}

void ProcessThings (void)
{
	Thing	        *wt;
	mapthing_t		mt;
	int				count;
	
    mapthingstore_i = NewArray(0, sizeof(mapthing_t), 1);

	count = thingstore_i->count;
	wt = Get(thingstore_i, 0);

	while ( count-- )
    {
		memset(&mt, 0, sizeof(mt));
		mt.x = wt->origin.x;
		mt.y = wt->origin.y;
		mt.angle = wt->angle;
		mt.type = wt->type;
		mt.options = wt->options;

        Push(mapthingstore_i, &mt);
		wt++;
	}
}

int ProcessSidedef(Sidedef * ws)
{
	mapsidedef_t ms;
	
	ms.textureoffset = ws->offsetX;
	ms.rowoffset = ws->offsetY;
	memcpy(ms.toptexture, ws->top, 8);
	memcpy(ms.bottomtexture, ws->bottom, 8);
	memcpy(ms.midtexture, ws->middle, 8);
	ms.sector = ws->sectorNum;

    Push(sdefstore_i, &ms);
	return sdefstore_i->count - 1;
}

/// - note: Must be called after `BuildSectors`.
void ProcessLineSideDefs (void)
{
	int				i, count;
	maplinedef_t	ld;
	Line		    *wl;
	
    mapvertexstore_i = NewArray(0, sizeof(mapvertex_t), 1);
    ldefstore_i = NewArray(0, sizeof(maplinedef_t), 1);
    sdefstore_i = NewArray(0, sizeof(mapsidedef_t), 1);

	count = linestore_i->count;
	wl = Get(linestore_i, 0);
	for (i=0 ; i<count ; i++, wl++)
	{
		ld.v1 = UniqueVertex(wl->p1.x, wl->p1.y);
		ld.v2 = UniqueVertex(wl->p2.x, wl->p2.y);
		ld.flags = wl->flags;
		ld.special = wl->special;
		ld.tag = wl->tag;
		ld.sidenum[0] = ProcessSidedef(&wl->sides[0]);

		if (wl->flags & ML_TWOSIDED)
			ld.sidenum[1] = ProcessSidedef(&wl->sides[1]);
		else
			ld.sidenum[1] =-1;

        Push(ldefstore_i, &ld);
	}
	
}

void SaveDoomMap (void)
{
	BuildSectordefs();
	ProcessThings();
	ProcessLineSideDefs();
	ProcessNodes();
	ProcessSectors();
	ProcessConnections();
	
    // all processing is complete, write everything out

	OutputThings();
	OutputLineDefs();
	OutputSideDefs();
	OutputVertexes();
	OutputSegs();
	OutputSubsectors();
	OutputNodes();
	OutputSectors();
	OutputConnections();
}
