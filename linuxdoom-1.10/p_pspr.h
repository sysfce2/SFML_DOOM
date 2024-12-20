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
// DESCRIPTION:
//  Sprite animation.
//
//-----------------------------------------------------------------------------

#pragma once

// Basic data types.
// Needs fixed point, and BAM angles.
#include "m_fixed.h"
#include "p_mobj.h"


//
// Frame flags:
// handles maximum brightness (torches, muzzle flare, light sources)
//
#define FF_FULLBRIGHT 0x8000 // flag in thing->frame
#define FF_FRAMEMASK 0x7fff

//
// Overlay psprites are scaled shapes
// drawn directly on the view screen,
// coordinates are given for a 320*200 view screen.
//
typedef enum {
  ps_weapon,
  ps_flash,
  NUMPSPRITES

} psprnum_t;

typedef struct {
  state_t *state; // a NULL state means not active
  int tics;
  fixed_t sx;
  fixed_t sy;

} pspdef_t;
