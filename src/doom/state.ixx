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
module;
#include <cstdint>
export module doom:state;

import :definitions;
import :player;

// Game Mode - identify IWAD as shareware, retail etc.
export game_mode gamemode = game_mode::indetermined;
export GameMission_t gamemission = doom;

// Language.
export Language_t language = english;

// Set if homebrew PWAD stuff has been added.
export bool modifiedgame;

export player_t *viewplayer;

// previously externs in doomstat.h
export inline int gamemap;
export inline int gametic;
export inline int gameepisode;
export inline bool playeringame[MAXPLAYERS];
export inline uint8_t deathmatch; // only if started as net death
export inline bool netgame;       // only true if packets are broadcast
export inline int consoleplayer;  // player taking events and displaying
export inline bool viewactive;
export inline bool singledemo; // quit after playing a demo from cmdline
export inline skill_t gameskill;
export inline int totalkills, totalitems, totalsecret; // for intermission
export inline bool usergame;                           // ok to save / end game
export inline bool demoplayback;
export inline gamestate_t gamestate;
export inline bool paused;
export inline int displayplayer; // view being displayed
export inline int bodyqueslot;
export inline bool precache = true; // if true, load all graphics at start
export inline bool nodrawers;       // for comparative timing purposes
export inline bool respawnmonsters;