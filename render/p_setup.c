// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Do all the WAD I/O, get map description,
//	set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------



#include <math.h>

//#include "z_zone.h"
//#include "m_swap.h"
#include "m_bbox.h"
//#include "g_game.h"
//#include "i_system.h"
//#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "doombsp.h"

//#include "s_sound.h"

//#include "doomstat.h"

#include "m_map.h"

void	P_SpawnMapThing (mapthing_t*	mthing);


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int		numvertexes;
vertex_t*	vertexes;

int		numsegs;
seg_t*		segs;

int		numsectors;
sector_t*	sectors;

int		numsubsectors;
subsector_t*	subsectors;

int		numnodes;
node_t*		nodes;

int		numlines;
rline_t*		lines;

int		numsides;
side_t*		sides;


// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int		bmapwidth;
int		bmapheight;	// size in mapblocks
short*		blockmap;	// int for larger maps
// offsets in blockmap are from here
short*		blockmaplump;		
// origin of block map
fixed_t		bmaporgx;
fixed_t		bmaporgy;
// for thing chains
mobj_t**	blocklinks;		


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte*		rejectmatrix;


// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS	10

mapthing_t	deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t*	deathmatch_p;
mapthing_t	playerstarts[MAXPLAYERS];





//
// P_LoadVertexes
//
void P_LoadVertexes (void)
{
    int			i;
    mapvertex_t*	ml;
    vertex_t*		li;

    if ( mapvertexstore_i->count > numvertexes )
    {
        vertexes = realloc(vertexes, mapvertexstore_i->count * sizeof(vertex_t));
    }

    // Determine number of lumps:
    //  total lump length / vertex record length.
//    numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);
    numvertexes = mapvertexstore_i->count;

    // Allocate zone memory for buffer.
//    vertexes = Z_Malloc (numvertexes*sizeof(vertex_t),PU_LEVEL,0);

    // Load data into cache.
//    data = W_CacheLumpNum (lump,PU_STATIC);
	
    ml = (mapvertex_t *)mapvertexstore_i->data;
    li = vertexes;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (i=0 ; i<numvertexes ; i++, li++, ml++)
    {
        li->x = ml->x<<FRACBITS;
        li->y = ml->y<<FRACBITS;
    }

    // Free buffer memory.
//    Z_Free (data);
}



//
// P_LoadSegs
//
void P_LoadSegs (void)
{
    int			i;
    mapseg_t*	ml;
    seg_t*		li;
    rline_t*		ldef;
    int			linedef;
    int			side;

//    numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
    if ( numsegs < maplinestore_i->count )
        segs = realloc(segs, maplinestore_i->count * sizeof(seg_t));
//    segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
    numsegs = maplinestore_i->count;
    memset (segs, 0, numsegs*sizeof(seg_t));
//    data = W_CacheLumpNum (lump,PU_STATIC);

    ml = (mapseg_t *)maplinestore_i->data;
    li = segs;
    for (i=0 ; i<numsegs ; i++, li++, ml++)
    {
        li->v1 = &vertexes[SWAP16(ml->v1)];
        li->v2 = &vertexes[SWAP16(ml->v2)];

        li->angle = (SWAP16(ml->angle))<<16;
        li->offset = (SWAP16(ml->offset))<<16;
        linedef = SWAP16(ml->linedef);
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = SWAP16(ml->side);
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;
        if (ldef-> flags & ML_TWOSIDED)
            li->backsector = sides[ldef->sidenum[side^1]].sector;
        else
            li->backsector = 0;
    }

//    Z_Free (data);
}


//
// P_LoadSubsectors
//
void P_LoadSubsectors (void)
{
    int			i;
    mapsubsector_t*	ms;
    subsector_t*	ss;

    if ( numsubsectors < subsecstore_i->count )
        subsectors = realloc(subsectors,
                             subsecstore_i->count * sizeof(subsector_t));
    numsubsectors = subsecstore_i->count;

//    numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
//    subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
//    data = W_CacheLumpNum (lump,PU_STATIC);
	
    ms = (mapsubsector_t *)subsecstore_i->data;
    memset (subsectors,0, numsubsectors*sizeof(subsector_t));
    ss = subsectors;
    
    for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
        ss->numlines = SWAP16(ms->numsegs);
        ss->firstline = SWAP16(ms->firstseg);
    }
	
//    Z_Free (data);
}



//
// P_LoadSectors
//
void P_LoadSectors (void)
{
    int			i;
    mapsector_t*	ms;
    sector_t*		ss;

    if ( numsectors < secstore_i->count )
        sectors = realloc(sectors, secstore_i->count * sizeof(sector_t));
    numsectors = secstore_i->count;

//    numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
//    sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);
//    memset (sectors, 0, numsectors*sizeof(sector_t));
//    data = W_CacheLumpNum (lump,PU_STATIC);
	
    ms = (mapsector_t *)secstore_i->data;
    ss = sectors;
    for (i=0 ; i<numsectors ; i++, ss++, ms++)
    {
        ss->floorheight = SWAP16(ms->floorheight)<<FRACBITS;
        ss->ceilingheight = SWAP16(ms->ceilingheight)<<FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = SWAP16(ms->lightlevel);
        ss->special = SWAP16(ms->special);
        ss->tag = SWAP16(ms->tag);
        ss->thinglist = NULL;
    }
	
//    Z_Free (data);
}


//
// P_LoadNodes
//
void P_LoadNodes (void)
{
    int		i;
    int		j;
    int		k;
    mapnode_t*	mn;
    node_t*	no;

    int count = nodestore_i->count;
    if ( numnodes < count )
        nodes = realloc(nodes, sizeof(node_t) * count);
    numnodes = count;

//    numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
//    nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);
//    data = W_CacheLumpNum (lump,PU_STATIC);
	
    mn = (mapnode_t *)nodestore_i->data;
    no = nodes;
    
    for (i=0 ; i<numnodes ; i++, no++, mn++)
    {
        no->x = SWAP16(mn->x)<<FRACBITS;
        no->y = SWAP16(mn->y)<<FRACBITS;
        no->dx = SWAP16(mn->dx)<<FRACBITS;
        no->dy = SWAP16(mn->dy)<<FRACBITS;
        for (j=0 ; j<2 ; j++)
        {
            no->children[j] = SWAP16(mn->children[j]);
            for (k=0 ; k<4 ; k++)
                no->bbox[j][k] = SWAP16(mn->bbox[j][k])<<FRACBITS;
        }
    }
}


//
// P_LoadThings
//
void P_LoadThings (void)
{
#if 0 // TODO: render things
    byte*		data;
    int			i;
    mapthing_t*		mt;
    int			numthings;
    boolean		spawn;

    data = W_CacheLumpNum (lump,PU_STATIC);
    numthings = W_LumpLength (lump) / sizeof(mapthing_t);

    mt = (mapthing_t *)data;
    for (i=0 ; i<numthings ; i++, mt++)
    {
        spawn = true;

        // Do not spawn cool, new monsters if !commercial
        if ( gamemode != commercial)
        {
            switch(mt->type)
            {
                case 68:	// Arachnotron
                case 64:	// Archvile
                case 88:	// Boss Brain
                case 89:	// Boss Shooter
                case 69:	// Hell Knight
                case 67:	// Mancubus
                case 71:	// Pain Elemental
                case 65:	// Former Human Commando
                case 66:	// Revenant
                case 84:	// Wolf SS
                    spawn = false;
                    break;
            }
        }
        if (spawn == false)
            break;

        // Do spawn all other stuff.
        mt->x = SWAP16(mt->x);
        mt->y = SWAP16(mt->y);
        mt->angle = SWAP16(mt->angle);
        mt->type = SWAP16(mt->type);
        mt->options = SWAP16(mt->options);

        P_SpawnMapThing (mt);
    }

    Z_Free (data);
#endif
}


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs (void)
{
    byte*		data;
    int			i;
    maplinedef_t*	mld;
    rline_t*		ld;
    vertex_t*		v1;
    vertex_t*		v2;

    if ( numlines < ldefstore_i->count )
        lines = realloc(lines, sizeof(rline_t) * ldefstore_i->count);
    numlines = ldefstore_i->count;
    data = ldefstore_i->data;
//    numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
//    lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);
//    memset (lines, 0, numlines*sizeof(line_t));
//    data = W_CacheLumpNum (lump,PU_STATIC);
	
    mld = (maplinedef_t *)data;
    ld = lines;
    for (i=0 ; i<numlines ; i++, mld++, ld++)
    {
        ld->flags = SWAP16(mld->flags);
        ld->special = SWAP16(mld->special);
        ld->tag = SWAP16(mld->tag);
        v1 = ld->v1 = &vertexes[SWAP16(mld->v1)];
        v2 = ld->v2 = &vertexes[SWAP16(mld->v2)];
        ld->dx = v2->x - v1->x;
        ld->dy = v2->y - v1->y;

        if (!ld->dx)
            ld->slopetype = ST_VERTICAL;
        else if (!ld->dy)
            ld->slopetype = ST_HORIZONTAL;
        else
        {
            if (FixedDiv (ld->dy , ld->dx) > 0)
                ld->slopetype = ST_POSITIVE;
            else
                ld->slopetype = ST_NEGATIVE;
        }

        if (v1->x < v2->x)
        {
            ld->bbox[BOXLEFT] = v1->x;
            ld->bbox[BOXRIGHT] = v2->x;
        }
        else
        {
            ld->bbox[BOXLEFT] = v2->x;
            ld->bbox[BOXRIGHT] = v1->x;
        }

        if (v1->y < v2->y)
        {
            ld->bbox[BOXBOTTOM] = v1->y;
            ld->bbox[BOXTOP] = v2->y;
        }
        else
        {
            ld->bbox[BOXBOTTOM] = v2->y;
            ld->bbox[BOXTOP] = v1->y;
        }

        ld->sidenum[0] = SWAP16(mld->sidenum[0]);
        ld->sidenum[1] = SWAP16(mld->sidenum[1]);

        if (ld->sidenum[0] != -1)
            ld->frontsector = sides[ld->sidenum[0]].sector;
        else
            ld->frontsector = 0;

        if (ld->sidenum[1] != -1)
            ld->backsector = sides[ld->sidenum[1]].sector;
        else
            ld->backsector = 0;
    }
}


//
// P_LoadSideDefs
//
void P_LoadSideDefs (void)
{
    byte*		data;
    int			i;
    mapsidedef_t*	msd;
    side_t*		sd;

    int count = sdefstore_i->count;

    if ( numsides < count )
        sides = realloc(sides, sizeof(side_t) * count);
    numsides = count;
    data = sdefstore_i->data;
//    numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
//    sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);
//    memset (sides, 0, numsides*sizeof(side_t));
//    data = W_CacheLumpNum (lump,PU_STATIC);
	
    msd = (mapsidedef_t *)data;
    sd = sides;
    for (i=0 ; i<numsides ; i++, msd++, sd++)
    {
        sd->textureoffset = SWAP16(msd->textureoffset)<<FRACBITS;
        sd->rowoffset = SWAP16(msd->rowoffset)<<FRACBITS;
        sd->toptexture = R_TextureNumForName(msd->toptexture);
        sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
        sd->midtexture = R_TextureNumForName(msd->midtexture);
        sd->sector = &sectors[SWAP16(msd->sector)];
    }

}


//
// P_LoadBlockMap
//
void P_LoadBlockMap (void)
{
#if 0
    // MARK: de removed
    int		i;
    int		count;
	
//    blockmaplump = W_CacheLumpNum (lump,PU_LEVEL);
//    blockmap = blockmaplump+4;
//    count = W_LumpLength (lump)/2;

    for (i=0 ; i<count ; i++)
        blockmaplump[i] = SWAP16(blockmaplump[i]);

    bmaporgx = blockmaplump[0]<<FRACBITS;
    bmaporgy = blockmaplump[1]<<FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];

    // clear out mobj chains
    count = sizeof(*blocklinks)* bmapwidth*bmapheight;
//    blocklinks = Z_Malloc (count,PU_LEVEL, 0);
//    memset (blocklinks, 0, count);
#endif
}



//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines (void)
{
    rline_t**		linebuffer;
    int			i;
    int			j;
    int			total;
    rline_t*		li;
    sector_t*		sector;
    subsector_t*	ss;
    seg_t*		seg;
    fixed_t		bbox[4];
    int			block;

    // look up sector number for each subsector
    ss = subsectors;
    for (i=0 ; i<numsubsectors ; i++, ss++)
    {
        seg = &segs[ss->firstline];
        ss->sector = seg->sidedef->sector;
    }

    // count number of lines in each sector
    li = lines;
    total = 0;
    for (i=0 ; i<numlines ; i++, li++)
    {
        total++;
        li->frontsector->linecount++;

        if (li->backsector && li->backsector != li->frontsector)
        {
            li->backsector->linecount++;
            total++;
        }
    }

    // build line tables for each sector
    //    linebuffer = Z_Malloc (total*4, PU_LEVEL, 0);

    // FIXME: confirm that 4 == sizeof(rline_t *) in original!
    linebuffer = malloc(total * sizeof(rline_t *));
    rline_t ** lbuf = linebuffer;

    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
        M_ClearBox (bbox);
//        sector->lines = linebuffer;
        sector->lines = lbuf;
        li = lines;
        for (j=0 ; j<numlines ; j++, li++)
        {
            if (li->frontsector == sector || li->backsector == sector)
            {
//                *linebuffer++ = li;
                *lbuf++ = li;
                M_AddToBox (bbox, li->v1->x, li->v1->y);
                M_AddToBox (bbox, li->v2->x, li->v2->y);
            }
        }
        if (lbuf - sector->lines != sector->linecount)
        {
            //        I_Error ("P_GroupLines: miscounted");
            // TODO: de error
        }

        // set the degenmobj_t to the middle of the bounding box
        sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
        sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;

        // adjust bounding box to map blocks
        block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block >= bmapheight ? bmapheight-1 : block;
        sector->blockbox[BOXTOP]=block;

        block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXBOTTOM]=block;

        block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block >= bmapwidth ? bmapwidth-1 : block;
        sector->blockbox[BOXRIGHT]=block;

        block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXLEFT]=block;
    }

    free(linebuffer);
}


//
// P_SetupLevel
//
#if 0
void
P_SetupLevel
( int		episode,
  int		map,
  int		playermask,
  skill_t	skill)
{
    int		i;
    char	lumpname[9];
    int		lumpnum;
	
    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 180;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	players[i].killcount = players[i].secretcount 
	    = players[i].itemcount = 0;
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1; 

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start ();			

    
#if 0 // UNUSED
    if (debugfile)
    {
	Z_FreeTags (PU_LEVEL, MAXINT);
	Z_FileDumpHeap (debugfile);
    }
    else
#endif
	Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);


    // UNUSED W_Profile ();
    P_InitThinkers ();

    // if working with a devlopment map, reload it
    W_Reload ();			
	   
    // find map name
    if ( gamemode == commercial)
    {
	if (map<10)
	    sprintf (lumpname,"map0%i", map);
	else
	    sprintf (lumpname,"map%i", map);
    }
    else
    {
	lumpname[0] = 'E';
	lumpname[1] = '0' + episode;
	lumpname[2] = 'M';
	lumpname[3] = '0' + map;
	lumpname[4] = 0;
    }

    lumpnum = W_GetNumForName (lumpname);
	
    leveltime = 0;
	
    // note: most of this ordering is important	
    P_LoadBlockMap (lumpnum+ML_BLOCKMAP);
    P_LoadVertexes (lumpnum+ML_VERTEXES);
    P_LoadSectors (lumpnum+ML_SECTORS);
    P_LoadSideDefs (lumpnum+ML_SIDEDEFS);

    P_LoadLineDefs (lumpnum+ML_LINEDEFS);
    P_LoadSubsectors (lumpnum+ML_SSECTORS);
    P_LoadNodes (lumpnum+ML_NODES);
    P_LoadSegs (lumpnum+ML_SEGS);
	
    rejectmatrix = W_CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);
    P_GroupLines ();

    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    P_LoadThings (lumpnum+ML_THINGS);
    
    // if deathmatch, randomly spawn the active players
    if (deathmatch)
    {
	for (i=0 ; i<MAXPLAYERS ; i++)
	    if (playeringame[i])
	    {
		players[i].mo = NULL;
		G_DeathMatchSpawnPlayer (i);
	    }
			
    }

    // clear special respawning que
    iquehead = iquetail = 0;		
	
    // set up world state
    P_SpawnSpecials ();
	
    // build subsector connect matrix
    //	UNUSED P_ConnectSubsectors ();

    // preload graphics
    if (precache)
	R_PrecacheLevel ();

    //printf ("free memory: 0x%x\n", Z_FreeMemory());

}
#endif

void LoadLevel(void)
{
    P_LoadBlockMap ();
    P_LoadVertexes ();
    P_LoadSectors ();
    P_LoadSideDefs ();
    P_LoadLineDefs ();
    P_LoadSubsectors ();
    P_LoadNodes ();
    P_LoadSegs ();

    P_GroupLines ();

    P_LoadThings ();

    for ( int i = 0; i < map.things->count; i++ )
    {
        Thing * thing = Get(map.things, i);

        if ( thing->type == 1 ) // Player 1 Start
        {
            viewPlayer.mo->x = thing->origin.x << FRACBITS;
            viewPlayer.mo->y = thing->origin.y << FRACBITS;
            viewPlayer.mo->z = 0; // TODO: Check this
            viewPlayer.mo->angle = ANG45 * (thing->angle / 45);
        }
    }
}


//
// P_Init
//
void P_Init (void)
{
//    P_InitSwitchList ();
//    P_InitPicAnims ();
    R_InitSprites (sprnames);
}



