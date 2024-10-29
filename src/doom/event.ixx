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
//
//
//-----------------------------------------------------------------------------
export module doom.event;

export enum class gameaction_t{
  ga_nothing,
  ga_loadlevel,
  ga_newgame,
  ga_loadgame,
  ga_savegame,
  ga_playdemo,
  ga_completed,
  ga_victory,
  ga_worlddone,
  ga_screenshot
};

export gameaction_t gameaction;

//
// Button/action code definitions.
//
export enum buttoncode_t : unsigned char{
  // Press "Fire".
  BT_ATTACK = 1,
  // Use button, to open doors, activate switches.
  BT_USE = 2,

  // Flag: game events, not really buttons.
  BT_SPECIAL = 128,
  BT_SPECIALMASK = 3,

  // Flag, weapon change pending.
  // If true, the next 3 bits hold weapon num.
  BT_CHANGE = 4,
  // The 3bit weapon mask and shift, convenience.
  BT_WEAPONMASK = (8 + 16 + 32),
  BT_WEAPONSHIFT = 3,

  // Pause the game.
  BTS_PAUSE = 1,
  // Save the game at each console.
  BTS_SAVEGAME = 2,

  // Savegame slot numbers
  //  occupy the second byte of buttons.
  BTS_SAVEMASK = (4 + 8 + 16),
  BTS_SAVESHIFT = 2,
};

//
// GLOBAL VARIABLES
//
export constexpr auto MAXEVENTS = 64;

//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
