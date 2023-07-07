// savebsp.m

#include "doombsp.h"

Array		*secdefstore_i;

#define		MAXVERTEX		8192
#define		MAXTOUCHSECS	16
#define		MAXSECTORS		2048
#define		MAXSUBSECTORS	2048

int			vertexsubcount[MAXVERTEX];
short		vertexsublist[MAXVERTEX][MAXTOUCHSECS];

int			subsectordef[MAXSUBSECTORS];
int			subsectornum[MAXSUBSECTORS];

int			buildsector;


/*
==========================
=
= RecursiveGroupSubsector
=
==========================
*/

void RecursiveGroupSubsector (int ssnum)
{
	int				i, l, count;
	int				vertex;
	int				checkss;
	short			*vertsub;
	int				vt;
	mapseg_t		*seg;
	mapsubsector_t	*ss;
	maplinedef_t	*ld;
	mapsidedef_t	*sd;
	
	ss = Get(subsecstore_i, ssnum);
	subsectornum[ssnum] = buildsector;
	
	for (l=0 ; l<ss->numsegs ; l++)
	{
		seg = Get(maplinestore_i, ss->firstseg+l);
		ld = Get(ldefstore_i, seg->linedef);
        DrawLineDef (ld);
		sd = Get(sdefstore_i, ld->sidenum[seg->side]);
		sd->sector = buildsector;
		
		for (vt=0 ; vt<2 ; vt++)
		{
			if (vt)
				vertex = seg->v1;
			else
				vertex = seg->v2;
			
			vertsub = vertexsublist[vertex];
			count = vertexsubcount[vertex];
			for (i=0 ; i<count ; i++)
			{
				checkss = vertsub[i];
				if (subsectordef[checkss] == subsectordef[ssnum])
				{
					if (subsectornum[checkss] == -1)
					{
						RecursiveGroupSubsector (checkss);
						continue;
					}
					if ( subsectornum[checkss] != buildsector)
						Error ("RecusiveGroup: regrouped a sector");
				}
			}
		}
	}
}

/*
=================
=
= UniqueSector
=
= Returns the sector number, adding a new sector if needed 
=================
*/

int UniqueSector(SectorDef * def)
{
	int		i, count;
	mapsector_t		ms, *msp;
	
	ms.floorheight = def->floorHeight;
	ms.ceilingheight = def->ceilingHeight;
	memcpy (ms.floorpic,def->floorFlat,8);
	memcpy (ms.ceilingpic,def->ceilingFlat,8);
	ms.lightlevel = def->lightLevel;
	ms.special = def->special;
	ms.tag = def->tag;
	
    // see if an identical sector already exists

	count = secdefstore_i->count;
	for (i=0 ; i<count ; i++)
    {
        msp = Get(secdefstore_i, i);
        if ( memcmp(msp, &ms, sizeof(ms)) == 0 )
//             if (!bcmp(msp, &ms, sizeof(ms)))
            return i;
    }

    Push(secdefstore_i, &ms);
	
	return count;	
}




void AddSubsectorToVertex (int subnum, int vertex)
{
	int		j;
	
	for (j=0 ; j< vertexsubcount[vertex] ; j++)
		if (vertexsublist[vertex][j] == subnum)
			return;
	vertexsublist[vertex][j] = subnum;
	vertexsubcount[vertex]++;
}


/*
================
=
= BuildSectordefs
=
= Call before ProcessNodes
================
*/

void BuildSectordefs (void)
{
	int				i;
	Line		    *wl;
	int				count;
	
    //
    // build sectordef list
    //

    secdefstore_i = NewArray(0, sizeof(mapsector_t), 1);
	
	count = linestore_i->count;
	wl= Get(linestore_i, 0);
	for (i=0 ; i<count ; i++, wl++)
	{
		wl->sides[0].sectorNum = UniqueSector(&wl->sides[0].sectorDef);

		if (wl->flags & ML_TWOSIDED)
			wl->sides[1].sectorNum = UniqueSector(&wl->sides[1].sectorDef);
	}
}


/*
================
=
= ProcessSectors
=
= Must be called after ProcessNodes, because it references the subsector list
================
*/

void ProcessSectors (void)
{
	int				i,l;
	int				numss;
	mapsubsector_t	*ss;	
	mapsector_t		sec;
	mapseg_t		*seg;
	maplinedef_t	*ml;
	mapsidedef_t	*ms;
	
//
// build a connection matrix that lists all the subsectors that touch
// each vertex
//
	memset (vertexsubcount, 0, sizeof(vertexsubcount));
	memset (vertexsublist, 0, sizeof(vertexsublist));
	numss = subsecstore_i->count;
	for (i=0 ; i<numss ; i++)
	{
		ss = Get(subsecstore_i, i);
		for (l=0 ; l<ss->numsegs ; l++)
		{
			seg = Get(maplinestore_i, ss->firstseg+l);
			AddSubsectorToVertex (i, seg->v1);
			AddSubsectorToVertex (i, seg->v2);
		}
		subsectornum[i] = -1;		// ungrouped
		ml = Get(ldefstore_i, seg->linedef);
		ms = Get(sdefstore_i, ml->sidenum[seg->side]);
		subsectordef[i] = ms->sector;
	}
	
    //
    // recursively build final sectors
    //

    secstore_i = NewArray(0, sizeof(mapsector_t), 1);
	
	buildsector = 0;
	if (draw)
        SDL_SetRenderDrawColor(nbRenderer, 0, 0, 0, 255);
//		PSsetgray (0); // TODO: nodebuilder draw
	for (i=0 ; i<numss ; i++)
	{
		if (subsectornum[i] == -1)
		{
            EraseWindow ();
			RecursiveGroupSubsector (i);
			sec = *(mapsector_t *)Get(secdefstore_i, subsectordef[i]);
            Push(secstore_i, &sec);
			buildsector++;
		}
	}

}


