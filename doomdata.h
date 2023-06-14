// DoomData.h

// all external data is defined here
// most of the data is loaded into different structures at run time

#ifndef __DOOMDATA__
#define __DOOMDATA__

#include "common.h"
#include <stdbool.h>

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef unsigned char byte;
#endif

/*
===============================================================================

                        map level types

===============================================================================
*/

// lump order in a map wad
enum
{
    ML_LABEL,
    ML_THINGS,
    ML_LINEDEFS,
    ML_SIDEDEFS,
    ML_VERTEXES,
    ML_SEGS,
    ML_SSECTORS,
    ML_NODES,
    ML_SECTORS,
    ML_REJECT,
    ML_BLOCKMAP,
    ML_COUNT
};


typedef struct
{
    s16 x, y;
} mapvertex_t;

typedef struct
{
    s16     textureoffset;
    s16     rowoffset;
    char    toptexture[8], bottomtexture[8], midtexture[8];
    s16     sector; // on viewer's side
} mapsidedef_t;

typedef struct
{
    s16     v1, v2;
    s16     flags;
    s16     special, tag;
    s16     sidenum[2]; // sidenum[1] will be -1 if one sided
} maplinedef_t;

#define ML_BLOCKING             1
#define ML_BLOCKMONSTERS        2
#define ML_TWOSIDED             4   // backside will not be present at all
                                    // if not two sided

// if a texture is pegged, the texture will have the end exposed to air held
// constant at the top or bottom of the texture (stairs or pulled down things)
// and will move with a height change of one of the neighbor sectors
// Unpegged textures allways have the first row of the texture at the top
// pixel of the line for both top and bottom textures (windows)
#define ML_DONTPEGTOP           8
#define ML_DONTPEGBOTTOM        16

#define ML_SECRET               32  // don't map as two sided: IT'S A SECRET!
#define ML_SOUNDBLOCK           64  // don't let sound cross two of these
#define ML_DONTDRAW             128 // don't draw on the automap
#define ML_MAPPED               256 // set if allready drawn in automap


typedef struct
{
    s16     floorheight, ceilingheight;
    char    floorpic[8], ceilingpic[8];
    s16     lightlevel;
    s16     special, tag;
} mapsector_t;

typedef struct
{
    s16     numsegs;
    s16     firstseg; // segs are stored sequentially
} mapsubsector_t;

typedef struct
{
    s16     v1, v2;
    s16     angle;
    s16     linedef, side;
    s16     offset;
} mapseg_t;

enum
{
    BOXTOP,
    BOXBOTTOM,
    BOXLEFT,
    BOXRIGHT
}; // bbox coordinates

#define NF_SUBSECTOR 0x8000
typedef struct
{
    s16     x, y, dx, dy;   // partition line
    s16     bbox[2][4];     // bounding box for each child
    u16     children[2];    // if NF_SUBSECTOR its a subsector
} mapnode_t;

typedef struct
{
    s16     x,y;
    s16     angle;
    s16     type;
    s16     options;
} mapthing_t;

#define MTF_EASY    1
#define MTF_NORMAL  2
#define MTF_HARD    4
#define MTF_AMBUSH  8

/*
===============================================================================

                        texture definition

===============================================================================
*/

typedef struct
{
    s16     originx;    // Horizontal location within texture.
    s16     originy;    // Vertical location within texture.
    s16     patch;      // Patch number in PNAMES lump.
    s16     stepdir;    // Unused
    s16     colormap;   // Unused
} mappatch_t;

typedef struct
{
    char        name[8];
    s32         masked; // OBSOLETE
    s16         width;
    s16         height;
    s32         columndirectory;    // OBSOLETE
    s16         patchcount;
    mappatch_t  patches[1];
} maptexture_t;


/*
===============================================================================

                            graphics

===============================================================================
*/

// posts are runs of non masked source pixels
typedef struct
{
    u8 topdelta; // -1 is the last post in a column
    u8 length;
    u8 padding; // Unused

// length data bytes follows
} post_t;

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;

// Structure within WAD (e.g. patches within P_START-P_END and S_START-S_END)
// a patch holds one or more columns
// patches are used for sprites and all masked pictures
typedef struct
{
    s16     width;          // bounding box size
    s16     height;
    s16     leftoffset;     // pixels to the left of origin
    s16     topoffset;      // pixels below the origin
    s32     columnofs[8];   // only [width] used
                            // the [0] is &columnofs[width]
} patch_t;

// a pic is an unmasked block of pixels
typedef struct
{
    byte    width, height;
    byte    data;
} pic_t;

/*
===============================================================================

                            status

===============================================================================
*/

#endif            // __DOOMDATA__

