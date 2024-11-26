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
//	Put all global state variables here.
//
//-----------------------------------------------------------------------------

export module doom:state;

import :definitions;
import :player;

// Game Mode - identify IWAD as shareware, retail etc.
export GameMode_t gamemode = indetermined;
export GameMission_t gamemission = doom;

// Language.
export Language_t language = english;

// Set if homebrew PWAD stuff has been added.
export bool modifiedgame;

export player_t *viewplayer;
