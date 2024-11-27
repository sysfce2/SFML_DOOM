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
module;
#include "g_game.h"
#include "m_bbox.h"
#include "p_local.h"

#include "r_data.h"
#include "r_things.h"

export module setup;

import engine;
import wad;
import doom;
import tick;
import sound;
import intermission;

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
export std::vector<vertex_t> vertexes;
export std::vector<seg_t> segs;
export std::vector<sector_t> sectors;
export std::vector<subsector_t> subsectors;
export std::vector<node_t> nodes;
export std::vector<line_t> lines;
export std::vector<side_t> sides;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
export int bmapwidth;
export int bmapheight;  // size in mapblocks
export short *blockmap; // int for larger maps
// offsets in blockmap are from here
export short *blockmaplump;
// origin of block map
export fixed_t bmaporgx;
export fixed_t bmaporgy;
// for thing chains
export mobj_t **blocklinks;

// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
export std::byte *rejectmatrix;

// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS 10

export mapthing_t deathmatchstarts[MAX_DEATHMATCH_STARTS];
export mapthing_t *deathmatch_p;
export mapthing_t playerstarts[MAXPLAYERS];

//
// P_LoadVertexes
//
void P_LoadVertexes(int lump)
{
    std::byte *data;
    mapvertex_t *ml;

    // Determine number of lumps:
    //  total lump length / vertex record length.
    auto numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);
    vertexes.resize(numvertexes);

    // Load data into cache.
    data = static_cast<std::byte *>(W_CacheLumpNum(lump));

    ml = (mapvertex_t *)data;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (auto &vert : vertexes)
    {
        vert.x = ml->x << FRACBITS;
        vert.y = ml->y << FRACBITS;
        ml++;
    }
}

//
// P_LoadSegs
//
void P_LoadSegs(int lump)
{
    std::byte *data;
    int i;
    mapseg_t *ml;
    seg_t *li;
    line_t *ldef;
    int linedef;
    int side;

    auto numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
    segs.resize(numsegs);
    data = static_cast<std::byte *>(W_CacheLumpNum(lump));

    ml = (mapseg_t *)data;
    for (auto &seg : segs)
    {
        seg.v1 = &vertexes[ml->v1];
        seg.v2 = &vertexes[ml->v2];

        seg.angle = (ml->angle) << 16;
        seg.offset = (ml->offset) << 16;
        linedef = ml->linedef;
        ldef = &lines[linedef];
        seg.linedef = ldef;
        side = ml->side;
        seg.sidedef = &sides[ldef->sidenum[side]];
        seg.frontsector = sides[ldef->sidenum[side]].sector;
        if (ldef->flags & ML_TWOSIDED)
            seg.backsector = sides[ldef->sidenum[side ^ 1]].sector;
        else
            seg.backsector = 0;
        ml++;
    }
}

//
// P_LoadSubsectors
//
void P_LoadSubsectors(int lump)
{
    std::byte *data;
    int i;
    mapsubsector_t *ms;
    subsector_t *ss;

    auto numsubsectors = W_LumpLength(lump) / sizeof(mapsubsector_t);
    subsectors.resize(numsubsectors);
    data = static_cast<std::byte *>(W_CacheLumpNum(lump));

    ms = (mapsubsector_t *)data;

    for (auto &ss : subsectors)
    {
        ss.numlines = ms->numsegs;
        ss.firstline = ms->firstseg;
        ms++;
    }
}

//
// P_LoadSectors
//
void P_LoadSectors(int lump)
{
    std::byte *data;
    int i;
    mapsector_t *ms;
    sector_t *ss;

    auto numsectors = W_LumpLength(lump) / sizeof(mapsector_t);
    sectors.resize(numsectors);
    data = static_cast<std::byte *>(W_CacheLumpNum(lump));

    ms = (mapsector_t *)data;
    ss = sectors.data();
    for (i = 0; i < numsectors; i++, ss++, ms++)
    {
        ss->floorheight = ms->floorheight << FRACBITS;
        ss->ceilingheight = ms->ceilingheight << FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = ms->lightlevel;
        ss->special = ms->special;
        ss->tag = ms->tag;
        ss->thinglist = NULL;
    }
}

//
// P_LoadNodes
//
void P_LoadNodes(int lump)
{
    std::byte *data;
    int j;
    int k;
    mapnode_t *mn;

    auto numnodes = W_LumpLength(lump) / sizeof(mapnode_t);
    nodes.resize(numnodes);
    data = static_cast<std::byte *>(W_CacheLumpNum(lump));

    mn = (mapnode_t *)data;

    for (auto &node : nodes)
    {
        node.x = mn->x << FRACBITS;
        node.y = mn->y << FRACBITS;
        node.dx = mn->dx << FRACBITS;
        node.dy = mn->dy << FRACBITS;
        for (j = 0; j < 2; j++)
        {
            node.children[j] = mn->children[j];
            for (k = 0; k < 4; k++)
                node.bbox[j][k] = mn->bbox[j][k] << FRACBITS;
        }
        mn++;
    }
}

//
// P_LoadThings
//
void P_LoadThings(int lump)
{
    std::byte *data;
    int i;
    mapthing_t *mt;
    int numthings;
    bool spawn;

    data = static_cast<std::byte *>(W_CacheLumpNum(lump));
    numthings = W_LumpLength(lump) / sizeof(mapthing_t);

    mt = (mapthing_t *)data;
    for (i = 0; i < numthings; i++, mt++)
    {
        spawn = true;

        // Do not spawn cool, new monsters if !commercial
        if (gamemode != commercial)
        {
            switch (mt->type)
            {
            case 68: // Arachnotron
            case 64: // Archvile
            case 88: // Boss Brain
            case 89: // Boss Shooter
            case 69: // Hell Knight
            case 67: // Mancubus
            case 71: // Pain Elemental
            case 65: // Former Human Commando
            case 66: // Revenant
            case 84: // Wolf SS
                spawn = false;
                break;
            }
        }
        if (spawn == false)
            break;

        // Do spawn all other stuff.
        mt->x = mt->x;
        mt->y = mt->y;
        mt->angle = mt->angle;
        mt->type = mt->type;
        mt->options = mt->options;

        P_SpawnMapThing(mt);
    }
}

//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs(int lump)
{
    std::byte *data;
    maplinedef_t *mld;
    vertex_t *v1;
    vertex_t *v2;

    auto numlines = W_LumpLength(lump) / sizeof(maplinedef_t);
    lines.resize(numlines);
    data = static_cast<std::byte *>(W_CacheLumpNum(lump));

    mld = (maplinedef_t *)data;
    for (auto &line : lines)
    {
        line.flags = mld->flags;
        line.special = mld->special;
        line.tag = mld->tag;
        v1 = line.v1 = &vertexes[mld->v1];
        v2 = line.v2 = &vertexes[mld->v2];
        line.dx = v2->x - v1->x;
        line.dy = v2->y - v1->y;

        if (!line.dx)
            line.slopetype = ST_VERTICAL;
        else if (!line.dy)
            line.slopetype = ST_HORIZONTAL;
        else
        {
            if (FixedDiv(line.dy, line.dx) > 0)
                line.slopetype = ST_POSITIVE;
            else
                line.slopetype = ST_NEGATIVE;
        }

        if (v1->x < v2->x)
        {
            line.bbox[BOXLEFT] = v1->x;
            line.bbox[BOXRIGHT] = v2->x;
        }
        else
        {
            line.bbox[BOXLEFT] = v2->x;
            line.bbox[BOXRIGHT] = v1->x;
        }

        if (v1->y < v2->y)
        {
            line.bbox[BOXBOTTOM] = v1->y;
            line.bbox[BOXTOP] = v2->y;
        }
        else
        {
            line.bbox[BOXBOTTOM] = v2->y;
            line.bbox[BOXTOP] = v1->y;
        }

        line.sidenum[0] = mld->sidenum[0];
        line.sidenum[1] = mld->sidenum[1];

        if (line.sidenum[0] != -1)
            line.frontsector = sides[line.sidenum[0]].sector;
        else
            line.frontsector = 0;

        if (line.sidenum[1] != -1)
            line.backsector = sides[line.sidenum[1]].sector;
        else
            line.backsector = 0;
        mld++;
    }
}

//
// P_LoadSideDefs
//
void P_LoadSideDefs(int lump)
{
    auto numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
    sides.resize(numsides);
    auto data = static_cast<std::byte *>(W_CacheLumpNum(lump));

    auto msd = (mapsidedef_t *)data;
    for (auto &sd : sides)
    {
        sd.textureoffset = msd->textureoffset << FRACBITS;
        sd.rowoffset = msd->rowoffset << FRACBITS;
        sd.toptexture = R_TextureNumForName(msd->toptexture);
        sd.bottomtexture = R_TextureNumForName(msd->bottomtexture);
        sd.midtexture = R_TextureNumForName(msd->midtexture);
        sd.sector = &sectors[msd->sector];
        msd++;
    }
}

//
// P_LoadBlockMap
//
void P_LoadBlockMap(int lump)
{
    int i;
    int count;

    blockmaplump = static_cast<short *>(W_CacheLumpNum(lump));
    blockmap = blockmaplump + 4;
    count = W_LumpLength(lump) / 2;

    for (i = 0; i < count; i++)
        blockmaplump[i] = blockmaplump[i];

    bmaporgx = blockmaplump[0] << FRACBITS;
    bmaporgy = blockmaplump[1] << FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];

    // clear out mobj chains
    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = static_cast<mobj_t **>(malloc(count));
    memset(blocklinks, 0, count);
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines(void)
{
    seg_t *seg;
    fixed_t bbox[4];
    int block;

    // look up sector number for each subsector
    for (auto i = 0; i < subsectors.size(); i++)
    {
        seg = &segs[subsectors[i].firstline];
        subsectors[i].sector = seg->sidedef->sector;
    }

    // build line tables for each sector
    for (auto &sector : sectors)
    {
        M_ClearBox(bbox);
        for (auto &line : lines)
        {
            if (line.frontsector == &sector || line.backsector == &sector)
            {
                sector.lines.push_back(&line);
                M_AddToBox(bbox, line.v1->x, line.v1->y);
                M_AddToBox(bbox, line.v2->x, line.v2->y);
            }
        }

        // set the degenmobj_t to the middle of the bounding box
        sector.soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT]) / 2;
        sector.soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM]) / 2;

        // adjust bounding box to map blocks
        block = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapheight ? bmapheight - 1 : block;
        sector.blockbox[BOXTOP] = block;

        block = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector.blockbox[BOXBOTTOM] = block;

        block = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapwidth ? bmapwidth - 1 : block;
        sector.blockbox[BOXRIGHT] = block;

        block = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector.blockbox[BOXLEFT] = block;
    }
}

//
// P_SetupLevel
//
export void P_SetupLevel(int episode, int map, int playermask, skill_t skill)
{
    int i;
    char lumpname[9];
    int lumpnum;

    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 180;
    for (i = 0; i < MAXPLAYERS; i++)
    {
        players[i].killcount = players[i].secretcount = players[i].itemcount =
            0;
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1;

    // Make sure all sounds are stopped before freeTags.
    S_Start();

    P_InitThinkers();

    // if working with a devlopment map, reload it
    W_Reload();

    // find map name
    if (gamemode == commercial)
    {
        if (map < 10)
            snprintf(lumpname, 9, "map0%i", map);
        else
            snprintf(lumpname, 9, "map%i", map);
    }
    else
    {
        lumpname[0] = 'E';
        lumpname[1] = '0' + episode;
        lumpname[2] = 'M';
        lumpname[3] = '0' + map;
        lumpname[4] = 0;
    }

    lumpnum = W_GetNumForName(lumpname);

    leveltime = 0;

    // note: most of this ordering is important
    P_LoadBlockMap(lumpnum + ML_BLOCKMAP);
    P_LoadVertexes(lumpnum + ML_VERTEXES);
    P_LoadSectors(lumpnum + ML_SECTORS);
    P_LoadSideDefs(lumpnum + ML_SIDEDEFS);

    P_LoadLineDefs(lumpnum + ML_LINEDEFS);
    P_LoadSubsectors(lumpnum + ML_SSECTORS);
    P_LoadNodes(lumpnum + ML_NODES);
    P_LoadSegs(lumpnum + ML_SEGS);

    rejectmatrix =
        static_cast<std::byte *>(W_CacheLumpNum(lumpnum + ML_REJECT));
    P_GroupLines();

    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    P_LoadThings(lumpnum + ML_THINGS);

    // if deathmatch, randomly spawn the active players
    if (deathmatch)
    {
        for (i = 0; i < MAXPLAYERS; i++)
            if (playeringame[i])
            {
                players[i].mo = NULL;
                G_DeathMatchSpawnPlayer(i);
            }
    }

    // clear special respawning que
    iquehead = iquetail = 0;

    // set up world state
    P_SpawnSpecials();

    // build subsector connect matrix
    //	UNUSED P_ConnectSubsectors ();

    // preload graphics
    if (precache)
        R_PrecacheLevel();

    // printf ("free memory: 0x%x\n", freeMemory());
}

//
// P_Init
//
export void P_Init(void)
{
    P_InitSwitchList();
    P_InitPicAnims();
    R_InitSprites(sprnames);
}
