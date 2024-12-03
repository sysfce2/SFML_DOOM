//-----------------------------------------------------------------------------
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
// DESCRIPTION:
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//
//-----------------------------------------------------------------------------
export module data;

// The most basic types we use, portability.

//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
export enum {
  ML_LABEL,    // A separator, name, ExMx or MAPxx
  ML_THINGS,   // Monsters, items..
  ML_LINEDEFS, // LineDefs, from editing
  ML_SIDEDEFS, // SideDefs, from editing
  ML_VERTEXES, // Vertices, edited and BSP splits generated
  ML_SEGS,     // LineSegs, from LineDefs split by BSP
  ML_SSECTORS, // SubSectors, list of LineSegs
  ML_NODES,    // BSP nodes
  ML_SECTORS,  // Sectors, from editing
  ML_REJECT,   // LUT, sector-sector visibility
  ML_BLOCKMAP  // LUT, motion clipping, walls/grid element
};

// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
typedef struct {
  short textureoffset;
  short rowoffset;
  char toptexture[8];
  char bottomtexture[8];
  char midtexture[8];
  // Front sector, towards viewer.
  short sector;
} mapsidedef_t;

// A LineDef, as used for editing, and as input
// to the BSP builder.
typedef struct {
  short v1;
  short v2;
  short flags;
  short special;
  short tag;
  // sidenum[1] will be -1 if one sided
  short sidenum[2];
} maplinedef_t;

//
// LineDef attributes.
//

// Solid, is an obstacle.
export constexpr auto ML_BLOCKING = 1;

// Blocks monsters only.
export constexpr auto ML_BLOCKMONSTERS = 2;

// Backside will not be present at all
//  if not two sided.
export constexpr auto ML_TWOSIDED = 4;

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures allways have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).

// upper texture unpegged
export constexpr auto ML_DONTPEGTOP = 8;

// lower texture unpegged
export constexpr auto ML_DONTPEGBOTTOM = 16;

// In AutoMap: don't map as two sided: IT'S A SECRET!
export constexpr auto ML_SECRET = 32;

// Sound rendering: don't let sound cross two of these.
export constexpr auto ML_SOUNDBLOCK = 64;

// Don't draw on the automap at all.
export constexpr auto ML_DONTDRAW = 128;

// Set if already seen, thus drawn in automap.
export constexpr auto ML_MAPPED = 256;

// Sector definition, from editing.
typedef struct {
  short floorheight;
  short ceilingheight;
  char floorpic[8];
  char ceilingpic[8];
  short lightlevel;
  short special;
  short tag;
} mapsector_t;

// BSP node structure.

// Indicate a leaf.
export constexpr auto NF_SUBSECTOR = 0x8000;

typedef struct {
  // Partition line from (x,y) to x+dx,y+dy)
  short x;
  short y;
  short dx;
  short dy;

  // Bounding box for each child,
  // clip against view frustum.
  short bbox[2][4];

  // If NF_SUBSECTOR its a subsector,
  // else it's a node of another subtree.
  unsigned short children[2];

} mapnode_t;

//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
