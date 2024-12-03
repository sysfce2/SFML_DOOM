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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------

#include <bitset>
#include <stdlib.h>
#include <string.h>


#include "m_random.h"


#include "p_saveg.h"


// Needs access to LFB.



#include "p_local.h"



// Data.


// SKY handling - still the wrong place.
#include "r_data.h"

#include "g_game.h"

import main;
import engine;
import menu;
import engine;
import wad;
import doom.map;
import net;
import finale;
import sky;
import tick;
import doom;
import setup;
import intermission;
import video;
import status_bar;
import sound;
import hud;
import info;
import app;

using enum gameaction_t;

#define SAVEGAMESIZE 0x2c000
#define SAVESTRINGSIZE 24

bool G_CheckDemoStatus(void);

void G_ReadDemoTiccmd(ticcmd_t *cmd);

void G_WriteDemoTiccmd(ticcmd_t *cmd);

void G_PlayerReborn(int player);

void G_InitNew(skill_t skill, int episode, int map);

void G_DoReborn(int playernum);

void G_DoLoadLevel(void);

void G_DoNewGame(void);

void G_DoLoadGame(void);

void G_DoPlayDemo(void);

void G_DoCompleted(void);

void G_DoVictory(void);

void G_DoWorldDone(void);

void G_DoSaveGame(void);

bool sendpause; // send a pause event next tic
bool sendsave;  // send a save event next tic

bool timingdemo; // if true, exit with report on completion
bool noblit;     // for comparative timing purposes
int starttime;      // for comparative timing purposes


int levelstarttic;                       // gametic at level start

std::filesystem::path demoname;
bool netdemo;
std::byte *demobuffer;
std::byte *demo_p;
std::byte *demoend;


short consistancy[MAXPLAYERS][BACKUPTICS];

std::byte *savebuffer;

//
// controls (have defaults)
//
auto key_right = static_cast<int>(sf::Keyboard::Scancode::Right);
auto key_left = static_cast<int>(sf::Keyboard::Scancode::Left);
auto key_up = static_cast<int>(sf::Keyboard::Scancode::Up);
auto key_down = static_cast<int>(sf::Keyboard::Scancode::Down);
auto key_strafeleft = static_cast<int>(sf::Keyboard::Scancode::Comma);
auto key_straferight = static_cast<int>(sf::Keyboard::Scancode::Period);
auto key_fire = static_cast<int>(sf::Keyboard::Scancode::LControl);
auto key_use = static_cast<int>(sf::Keyboard::Scancode::Space);
auto key_strafe = static_cast<int>(sf::Keyboard::Scancode::RAlt);
auto key_speed = static_cast<int>(sf::Keyboard::Scancode::RShift);

auto mousebfire = static_cast<int>(sf::Mouse::Button::Left);
auto mousebstrafe = static_cast<int>(sf::Mouse::Button::Right);
auto mousebforward = static_cast<int>(sf::Mouse::Button::Middle);

auto joybfire = 0;
auto joybstrafe = 1;
auto joybuse = 3;
auto joybspeed = 2;

#define MAXPLMOVE (forwardmove[1])

#define TURBOTHRESHOLD 0x32

fixed_t angleturn[3] = {640, 1280, 320}; // + slow turn

#define SLOWTURNTICS 6

std::bitset<sf::Keyboard::ScancodeCount> gamekeydown;
int turnheld; // for accelerative turning

std::bitset<sf::Mouse::ButtonCount> mousebuttons;

// mouse values are used once
int mousex;
int mousey;

int dclicktime;
int dclickstate;
int dclicks;
int dclicktime2;
int dclickstate2;
int dclicks2;

// joystick values are repeated
int joyxmove;
int joyymove;

std::bitset<sf::Joystick::ButtonCount> joybuttons;

int savegameslot;
char savedescription[32];

#define BODYQUESIZE 32

mobj_t *bodyque[BODYQUESIZE];


void *statcopy; // for statistics driver

int G_CmdChecksum(ticcmd_t *cmd) {
  int i;
  int sum = 0;

  for (i = 0; i < sizeof(*cmd) / 4 - 1; i++)
    sum += ((int *)cmd)[i];

  return sum;
}

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd(ticcmd_t *cmd) {
  int i;
  bool strafe;
  bool bstrafe;
  int speed;
  int tspeed;
  int forward;
  int side;

  cmd->consistancy = consistancy[consoleplayer][maketic % BACKUPTICS];

  strafe = gamekeydown[key_strafe] || mousebuttons[mousebstrafe] ||
           joybuttons[joybstrafe];
  speed = gamekeydown[key_speed] || joybuttons[joybspeed];

  forward = side = 0;

  // use two stage accelerative turning
  // on the keyboard and joystick
  if (joyxmove < 0 || joyxmove > 0 || gamekeydown[key_right] ||
      gamekeydown[key_left])
    turnheld += ticdup;
  else
    turnheld = 0;

  if (turnheld < SLOWTURNTICS)
    tspeed = 2; // slow turn
  else
    tspeed = speed;

  // let movement keys cancel each other out
  if (strafe) {
    if (gamekeydown[key_right]) {
      // fprintf(stderr, "strafe right\n");
      side += sidemove[speed];
    }
    if (gamekeydown[key_left]) {
      //	fprintf(stderr, "strafe left\n");
      side -= sidemove[speed];
    }
    if (joyxmove > 0)
      side += sidemove[speed];
    if (joyxmove < 0)
      side -= sidemove[speed];

  } else {
    if (gamekeydown[key_right])
      cmd->angleturn -= angleturn[tspeed];
    if (gamekeydown[key_left])
      cmd->angleturn += angleturn[tspeed];
    if (joyxmove > 0)
      cmd->angleturn -= angleturn[tspeed];
    if (joyxmove < 0)
      cmd->angleturn += angleturn[tspeed];
  }

  if (gamekeydown[key_up]) {
    // fprintf(stderr, "up\n");
    forward += forwardmove[speed];
  }
  if (gamekeydown[key_down]) {
    // fprintf(stderr, "down\n");
    forward -= forwardmove[speed];
  }
  if (joyymove < 0)
    forward += forwardmove[speed];
  if (joyymove > 0)
    forward -= forwardmove[speed];
  if (gamekeydown[key_straferight])
    side += sidemove[speed];
  if (gamekeydown[key_strafeleft])
    side -= sidemove[speed];

  // buttons
  cmd->chatchar = HU_dequeueChatChar();

  if (gamekeydown[key_fire] || mousebuttons[mousebfire] || joybuttons[joybfire])
    cmd->buttons = static_cast<buttoncode_t>(cmd->buttons | BT_ATTACK);

  if (gamekeydown[key_use] || joybuttons[joybuse]) {
    cmd->buttons = static_cast<buttoncode_t>(cmd->buttons | BT_USE);
    // clear double clicks if hit use button
    dclicks = 0;
  }

  // chainsaw overrides
  for (i = 0; i < NUMWEAPONS - 1; i++)
    if (gamekeydown['1' + i]) {
      cmd->buttons = static_cast<buttoncode_t>(cmd->buttons | BT_CHANGE);
      cmd->buttons = static_cast<buttoncode_t>(cmd->buttons | (i << BT_WEAPONSHIFT));
      break;
    }

  // mouse
  if (mousebuttons[mousebforward])
    forward += forwardmove[speed];

  // forward double click
  if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1) {
    dclickstate = mousebuttons[mousebforward];
    if (dclickstate)
      dclicks++;
    if (dclicks == 2) {
      cmd->buttons = static_cast<buttoncode_t>(cmd->buttons | BT_USE);
      dclicks = 0;
    } else
      dclicktime = 0;
  } else {
    dclicktime += ticdup;
    if (dclicktime > 20) {
      dclicks = 0;
      dclickstate = 0;
    }
  }

  // strafe double click
  bstrafe = mousebuttons[mousebstrafe] || joybuttons[joybstrafe];
  if ( int{ bstrafe } != dclickstate2 && dclicktime2 > 1 )
  {
    dclickstate2 = bstrafe;
    if (dclickstate2)
      dclicks2++;
    if (dclicks2 == 2) {
      cmd->buttons = static_cast<buttoncode_t>(cmd->buttons | BT_USE);
      dclicks2 = 0;
    } else
      dclicktime2 = 0;
  } else {
    dclicktime2 += ticdup;
    if (dclicktime2 > 20) {
      dclicks2 = 0;
      dclickstate2 = 0;
    }
  }

  forward += mousey;
  if (strafe)
    side += mousex * 2;
  else
    cmd->angleturn -= mousex * 0x8;

  mousex = mousey = 0;

  if (forward > MAXPLMOVE)
    forward = MAXPLMOVE;
  else if (forward < -MAXPLMOVE)
    forward = -MAXPLMOVE;
  if (side > MAXPLMOVE)
    side = MAXPLMOVE;
  else if (side < -MAXPLMOVE)
    side = -MAXPLMOVE;

  cmd->forwardmove += forward;
  cmd->sidemove += side;

  // special buttons
  if (sendpause) {
    sendpause = false;
    cmd->buttons = static_cast<buttoncode_t>(BT_SPECIAL | BTS_PAUSE);
  }

  if (sendsave) {
    sendsave = false;
    cmd->buttons = static_cast<buttoncode_t>(BT_SPECIAL | BTS_SAVEGAME | (savegameslot << BTS_SAVESHIFT));
  }
}

//
// G_DoLoadLevel
//
void G_DoLoadLevel(void) {
  int i;

  // Set the sky map.
  // First thing, we have a dummy sky texture name,
  //  a flat. The data is in the WAD only because
  //  we look for an actual index, instead of simply
  //  setting one.
  skyflatnum = R_FlatNumForName(SKYFLATNAME);

  // DOOM determines the sky texture to be used
  // depending on the current episode, and the game version.
  if ((gamemode == commercial) || (gamemission == pack_tnt) ||
      (gamemission == pack_plut)) {
    skytexture = R_TextureNumForName("SKY3");
    if (gamemap < 12)
      skytexture = R_TextureNumForName("SKY1");
    else if (gamemap < 21)
      skytexture = R_TextureNumForName("SKY2");
  }

  levelstarttic = gametic; // for time calculation

  if (wipegamestate == GS_LEVEL)
    wipegamestate = static_cast<gamestate_t>(-1); // force a wipe

  gamestate = GS_LEVEL;

  for (i = 0; i < MAXPLAYERS; i++) {
    if (playeringame[i] && players[i].playerstate == PST_DEAD)
      players[i].playerstate = PST_REBORN;
    memset(players[i].frags, 0, sizeof(players[i].frags));
  }

  P_SetupLevel(gameepisode, gamemap, 0, gameskill);
  displayplayer = consoleplayer; // view the guy you are playing
  starttime = I_GetTime();
  gameaction = ga_nothing;

  // clear cmd building stuff
  gamekeydown = {};
  joyxmove = joyymove = 0;
  mousex = mousey = 0;
  sendpause = sendsave = paused = false;
}

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//

bool G_Responder(const sf::Event &ev) {
  // allow spy mode changes even during the demo
  if (auto key_press = ev.getIf<sf::Event::KeyPressed>();
      key_press && gamestate == GS_LEVEL &&
      key_press->code == sf::Keyboard::Key::F12 &&
      (singledemo || !deathmatch)) {
    // spy mode
    do {
      displayplayer++;
      if (displayplayer == MAXPLAYERS)
        displayplayer = 0;
    } while (!playeringame[displayplayer] && displayplayer != consoleplayer);
    return true;
  }

  // any other key pops up menu if in demos
  if (gameaction == ga_nothing && !singledemo &&
      (demoplayback || gamestate == GS_DEMOSCREEN)) {
    if (ev.is<sf::Event::KeyPressed>() || ev.is<sf::Event::MouseMoved>() ||
        ev.is<sf::Event::JoystickButtonPressed>()) {
      M_StartControlPanel();
      return true;
    }
    return false;
  }

  if (gamestate == GS_LEVEL) {
    if (HU_Responder(ev))
      return true; // chat ate the event
    if (ST_Responder(ev))
      return true; // status window ate it
    if (AM_Responder(ev))
      return true; // automap ate it
  }

  if (gamestate == GS_FINALE) {
    if (F_Responder(ev))
      return true; // finale ate the event
  }

  if (auto key_press = ev.getIf<sf::Event::KeyPressed>(); key_press) {
    // JONNY TODO
    //	if (ev.key.code == KEY_PAUSE)
    //	{
    //	    sendpause = true;
    //	    return true;
    //	}
    gamekeydown[static_cast<int>(key_press->scancode)] = true;
    return true; // eat key down events
  } else if (auto key_release = ev.getIf<sf::Event::KeyReleased>();
             key_release) {

    gamekeydown[static_cast<int>(key_release->scancode)] = false;
    return false; // always let key up events filter down
  } else if (auto mouse_press = ev.getIf<sf::Event::MouseButtonPressed>();
             mouse_press) {
    mousebuttons[static_cast<int>(mouse_press->button)] = true;
  } else if (auto mouse_release = ev.getIf<sf::Event::MouseButtonReleased>();
             mouse_press) {
    mousebuttons[static_cast<int>(mouse_press->button)] = false;
  } else if (auto mouse_move = ev.getIf<sf::Event::MouseMoved>(); mouse_move) {
    mousex = mouse_move->position.x * (mouseSensitivity + 5) / 10;
    mousey = mouse_move->position.y * (mouseSensitivity + 5) / 10;
    return true; // eat events
  } else if (auto joystick_press_event =
                 ev.getIf<sf::Event::JoystickButtonPressed>();
             joystick_press_event) {
    joybuttons[joystick_press_event->button] = true;
  } else if (auto joystick_release_event =
                 ev.getIf<sf::Event::JoystickButtonReleased>();
             joystick_release_event) {
    joybuttons[joystick_release_event->button] = false;
  } else if (auto joystick_move_event = ev.getIf<sf::Event::JoystickMoved>();
             joystick_move_event) {
    switch (joystick_move_event->axis) {
    case sf::Joystick::Axis::X:
      joyxmove = joystick_move_event->position;
      break;
    case sf::Joystick::Axis::Y:
      joyymove = joystick_move_event->position;
      break;
    default:
      break;
    }
    return true; // eat events
  }

  return false;
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker(void) {
  int i;
  int buf;
  ticcmd_t *cmd;

  // do player reborns if needed
  for (i = 0; i < MAXPLAYERS; i++)
    if (playeringame[i] && players[i].playerstate == PST_REBORN)
      G_DoReborn(i);

  // do things to change the game state
  while (gameaction != ga_nothing) {
    switch (gameaction) {
    case ga_loadlevel:
      G_DoLoadLevel();
      break;
    case ga_newgame:
      G_DoNewGame();
      break;
    case ga_loadgame:
      G_DoLoadGame();
      break;
    case ga_savegame:
      G_DoSaveGame();
      break;
    case ga_playdemo:
      G_DoPlayDemo();
      break;
    case ga_completed:
      G_DoCompleted();
      break;
    case ga_victory:
      F_StartFinale();
      break;
    case ga_worlddone:
      G_DoWorldDone();
      break;
    case ga_screenshot:
      // @ TODO JONNY implement this somewhere
      //M_ScreenShot();
      gameaction = ga_nothing;
      break;
    case ga_nothing:
      break;
    }
  }

  // get commands, check consistancy,
  // and build new consistancy check
  buf = (gametic / ticdup) % BACKUPTICS;

  for (i = 0; i < MAXPLAYERS; i++) {
    if (playeringame[i]) {
      cmd = &players[i].cmd;

      memcpy(cmd, &netcmds[i][buf], sizeof(ticcmd_t));

      if (demoplayback)
        G_ReadDemoTiccmd(cmd);
      if (demorecording)
        G_WriteDemoTiccmd(cmd);

      // check for turbo cheats
      if (cmd->forwardmove > TURBOTHRESHOLD && !(gametic & 31) &&
          ((gametic >> 5) & 3) == i) {
        static char turbomessage[80];
        snprintf(turbomessage, 80, "%s is turbo!", player_names[i]);
        players[consoleplayer].message = turbomessage;
      }

      if (netgame && !netdemo && !(gametic % ticdup)) {
        if (gametic > BACKUPTICS && consistancy[i][buf] != cmd->consistancy) {
          logger::error("consistency failure ({} should be {})", cmd->consistancy,
                  consistancy[i][buf]);
        }
        if (players[i].mo)
          consistancy[i][buf] = players[i].mo->x;
        //else 
          // TODO JOONNY rndindex needs to be visible here
          //consistancy[i][buf] = rndindex;
      }
    }
  }

  // check for special buttons
  for (i = 0; i < MAXPLAYERS; i++) {
    if (playeringame[i]) {
      if (players[i].cmd.buttons & BT_SPECIAL) {
        switch (players[i].cmd.buttons & BT_SPECIALMASK) {
        case BTS_PAUSE:
          paused ^= 1;
          if (paused)
            S_PauseSound();
          else
            S_ResumeSound();
          break;

        case BTS_SAVEGAME:
          if (!savedescription[0])
            strcpy(savedescription, "NET GAME");
          savegameslot =
              (players[i].cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT;
          gameaction = ga_savegame;
          break;
        }
      }
    }
  }

  // do main actions
  switch (gamestate) {
  case GS_LEVEL:
    P_Ticker();
    ST_Ticker();
    AM_Ticker();
    HU_Ticker();
    break;

  case GS_INTERMISSION:
    WI_Ticker();
    break;

  case GS_FINALE:
    F_Ticker();
    break;

  case GS_DEMOSCREEN:
    D_PageTicker();
    break;
  }
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer
// Called at the start.
// Called by the game initialization functions.
//
void G_InitPlayer(int player) {
  // clear everything else to defaults
  G_PlayerReborn(player);
}

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel(int player) {
  player_t *p;

  p = &players[player];

  memset(p->powers, 0, sizeof(p->powers));
  memset(p->cards, 0, sizeof(p->cards));
  p->mo->flags &= ~MF_SHADOW; // cancel invisibility
  p->extralight = 0;          // cancel gun flashes
  p->fixedcolormap = 0;       // cancel ir gogles
  p->damagecount = 0;         // no palette changes
  p->bonuscount = 0;
}

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn(int player) {
  player_t *p;
  int i;
  int frags[MAXPLAYERS];
  int killcount;
  int itemcount;
  int secretcount;

  memcpy(frags, players[player].frags, sizeof(frags));
  killcount = players[player].killcount;
  itemcount = players[player].itemcount;
  secretcount = players[player].secretcount;

  p = &players[player];
  memset(p, 0, sizeof(*p));

  memcpy(players[player].frags, frags, sizeof(players[player].frags));
  players[player].killcount = killcount;
  players[player].itemcount = itemcount;
  players[player].secretcount = secretcount;

  p->usedown = p->attackdown = true; // don't do anything immediately
  p->playerstate = PST_LIVE;
  p->health = MAXHEALTH;
  p->readyweapon = p->pendingweapon = wp_pistol;
  p->weaponowned[wp_fist] = true;
  p->weaponowned[wp_pistol] = true;
  p->ammo[am_clip] = 50;

  for (i = 0; i < NUMAMMO; i++)
    p->maxammo[i] = maxammo[i];
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
void P_SpawnPlayer(mapthing_t *mthing);

bool G_CheckSpot(int playernum, mapthing_t *mthing) {
  fixed_t x;
  fixed_t y;
  subsector_t *ss;
  unsigned an;
  mobj_t *mo;
  int i;

  if (!players[playernum].mo) {
    // first spawn of level, before corpses
    for (i = 0; i < playernum; i++)
      if (players[i].mo->x == mthing->x << FRACBITS &&
          players[i].mo->y == mthing->y << FRACBITS)
        return false;
    return true;
  }

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  if (!P_CheckPosition(players[playernum].mo, x, y))
    return false;

  // flush an old corpse if needed
  if (bodyqueslot >= BODYQUESIZE)
    P_RemoveMobj(bodyque[bodyqueslot % BODYQUESIZE]);
  bodyque[bodyqueslot % BODYQUESIZE] = players[playernum].mo;
  bodyqueslot++;

  // spawn a teleport fog
  ss = R_PointInSubsector(x, y);
  an = (ANG45 * (mthing->angle / 45)) >> ANGLETOFINESHIFT;

  mo = P_SpawnMobj(x + 20 * finecosine[an], y + 20 * finesine[an],
                   ss->sector->floorheight, MT_TFOG);

  if (players[consoleplayer].viewz != 1)
    S_StartSound(mo, sfx_telept); // don't start sound on first frame

  return true;
}

//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//
void G_DeathMatchSpawnPlayer(int playernum) {
  int i, j;

  const auto selections = deathmatch_p - deathmatchstarts;
  if (selections < 4)
    logger::error("Only {} deathmatch spots, 4 required", selections);

  for (j = 0; j < 20; j++) {
    i = P_Random() % selections;
    if (G_CheckSpot(playernum, &deathmatchstarts[i])) {
      deathmatchstarts[i].type = playernum + 1;
      P_SpawnPlayer(&deathmatchstarts[i]);
      return;
    }
  }

  // no good spot, so the player will probably get stuck
  P_SpawnPlayer(&playerstarts[playernum]);
}

//
// G_DoReborn
//
void G_DoReborn(int playernum) {
  int i;

  if (!netgame) {
    // reload the level from scratch
    gameaction = ga_loadlevel;
  } else {
    // respawn at the start

    // first dissasociate the corpse
    players[playernum].mo->player = NULL;

    // spawn at random spot if in death match
    if (deathmatch) {
      G_DeathMatchSpawnPlayer(playernum);
      return;
    }

    if (G_CheckSpot(playernum, &playerstarts[playernum])) {
      P_SpawnPlayer(&playerstarts[playernum]);
      return;
    }

    // try to spawn at one of the other players spots
    for (i = 0; i < MAXPLAYERS; i++) {
      if (G_CheckSpot(playernum, &playerstarts[i])) {
        playerstarts[i].type = playernum + 1; // fake as other player
        P_SpawnPlayer(&playerstarts[i]);
        playerstarts[i].type = i + 1; // restore
        return;
      }
      // he's going to be inside something.  Too bad.
    }
    P_SpawnPlayer(&playerstarts[playernum]);
  }
}

void G_ScreenShot(void) { gameaction = ga_screenshot; }

// DOOM Par Times
int pars[4][10] = {{0},
                   {0, 30, 75, 120, 90, 165, 180, 180, 30, 165},
                   {0, 90, 90, 90, 120, 90, 360, 240, 30, 170},
                   {0, 90, 45, 90, 150, 90, 90, 165, 30, 135}};

// DOOM II Par Times
int cpars[32] = {
    30,  90,  120, 120, 90,  150, 120, 120, 270, 90,  //  1-10
    210, 150, 150, 150, 210, 150, 420, 150, 210, 150, // 11-20
    240, 150, 180, 150, 150, 300, 330, 420, 300, 180, // 21-30
    120, 30                                           // 31-32
};

//
// G_DoCompleted
//
bool secretexit;

void G_ExitLevel(void) {
  secretexit = false;
  gameaction = ga_completed;
}

// Here's for the german edition.
void G_SecretExitLevel(void) {
  // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ( ( gamemode == commercial ) && ( wad::index_of( "map31" ) < 0 ) )
    secretexit = false;
  else
    secretexit = true;
  gameaction = ga_completed;
}

void G_DoCompleted(void) {
  int i;

  gameaction = ga_nothing;

  for (i = 0; i < MAXPLAYERS; i++)
    if (playeringame[i])
      G_PlayerFinishLevel(i); // take away cards and stuff

  if (automapactive)
    AM_Stop();

  if (gamemode != commercial)
    switch (gamemap) {
    case 8:
      gameaction = ga_victory;
      return;
    case 9:
      for (i = 0; i < MAXPLAYERS; i++)
        players[i].didsecret = true;
      break;
    }

  // #if 0  Hmmm - why?
  if ((gamemap == 8) && (gamemode != commercial)) {
    // victory
    gameaction = ga_victory;
    return;
  }

  if ((gamemap == 9) && (gamemode != commercial)) {
    // exit secret level
    for (i = 0; i < MAXPLAYERS; i++)
      players[i].didsecret = true;
  }
  // #endif

  wminfo.didsecret = players[consoleplayer].didsecret;
  wminfo.epsd = gameepisode - 1;
  wminfo.last = gamemap - 1;

  // wminfo.next is 0 biased, unlike gamemap
  if (gamemode == commercial) {
    if (secretexit)
      switch (gamemap) {
      case 15:
        wminfo.next = 30;
        break;
      case 31:
        wminfo.next = 31;
        break;
      }
    else
      switch (gamemap) {
      case 31:
      case 32:
        wminfo.next = 15;
        break;
      default:
        wminfo.next = gamemap;
      }
  } else {
    if (secretexit)
      wminfo.next = 8; // go to secret level
    else if (gamemap == 9) {
      // returning from secret level
      switch (gameepisode) {
      case 1:
        wminfo.next = 3;
        break;
      case 2:
        wminfo.next = 5;
        break;
      case 3:
        wminfo.next = 6;
        break;
      case 4:
        wminfo.next = 2;
        break;
      }
    } else
      wminfo.next = gamemap; // go to next level
  }

  wminfo.maxkills = totalkills;
  wminfo.maxitems = totalitems;
  wminfo.maxsecret = totalsecret;
  wminfo.maxfrags = 0;
  if (gamemode == commercial)
    wminfo.partime = 35 * cpars[gamemap - 1];
  else
    wminfo.partime = 35 * pars[gameepisode][gamemap];
  wminfo.pnum = consoleplayer;

  for (i = 0; i < MAXPLAYERS; i++) {
    wminfo.plyr[i].in = playeringame[i];
    wminfo.plyr[i].skills = players[i].killcount;
    wminfo.plyr[i].sitems = players[i].itemcount;
    wminfo.plyr[i].ssecret = players[i].secretcount;
    wminfo.plyr[i].stime = leveltime;
    memcpy(wminfo.plyr[i].frags, players[i].frags,
           sizeof(wminfo.plyr[i].frags));
  }

  gamestate = GS_INTERMISSION;
  viewactive = false;
  automapactive = false;

  if (statcopy)
    memcpy(statcopy, &wminfo, sizeof(wminfo));

  WI_Start(&wminfo);
}

//
// G_WorldDone
//
void G_WorldDone(void) {
  gameaction = ga_worlddone;

  if (secretexit)
    players[consoleplayer].didsecret = true;

  if (gamemode == commercial) {
    switch (gamemap) {
    case 15:
    case 31:
      if (!secretexit)
        break;
    case 6:
    case 11:
    case 20:
    case 30:
      F_StartFinale();
      break;
    }
  }
}

void G_DoWorldDone(void) {
  gamestate = GS_LEVEL;
  gamemap = wminfo.next + 1;
  G_DoLoadLevel();
  gameaction = ga_nothing;
  viewactive = true;
}

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//

std::string savename;

void G_LoadGame(const std::string& name) {
  savename = name;
  gameaction = ga_loadgame;
}

#define VERSIONSIZE 16

void G_DoLoadGame(void) {
  int i;
  int a, b, c;
  char vcheck[VERSIONSIZE];

  gameaction = ga_nothing;

  // M_ReadFile (savename, &savebuffer);
  save_p = savebuffer + SAVESTRINGSIZE;

  // skip the description field
  memset(vcheck, 0, sizeof(vcheck));
  snprintf(vcheck, VERSIONSIZE, "version %i", VERSION);
  // JONNY TODO
  // if (strcmp (save_p, vcheck))
  // return;				// bad version
  save_p += VERSIONSIZE;

  gameskill = static_cast<skill_t>(*save_p++);
  gameepisode = static_cast<int>(*save_p++);
  gamemap = static_cast<int>(*save_p++);
  for (i = 0; i < MAXPLAYERS; i++)
    playeringame[i] = static_cast<bool>(*save_p++);

  // load a base level
  G_InitNew(gameskill, gameepisode, gamemap);

  // get the times
  a = static_cast<int>(*save_p++);
  b = static_cast<int>(*save_p++);
  c = static_cast<int>(*save_p++);
  leveltime = (a << 16) + (b << 8) + c;

  // dearchive all the modifications
  P_UnArchivePlayers();
  P_UnArchiveWorld();
  P_UnArchiveThinkers();
  P_UnArchiveSpecials();

  if (*save_p != std::byte{0x1d})
    logger::error("Bad savegame");

  // done
  free(savebuffer);

  if (setsizeneeded)
    R_ExecuteSetViewSize();

  // draw the pattern into the back screen
  R_FillBackScreen();
}

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void G_SaveGame(int slot, char *description) {
  savegameslot = slot;
  strcpy(savedescription, description);
  sendsave = true;
}

void G_DoSaveGame(void) {
  char name[100];
  char name2[VERSIONSIZE];
  char *description;
  int i;

  //snprintf(name, 100, SAVEGAMENAME "%d.dsg", savegameslot);
  description = savedescription;

  save_p = savebuffer = screens[1].data() + 0x4000;

  memcpy(save_p, description, SAVESTRINGSIZE);
  save_p += SAVESTRINGSIZE;
  memset(name2, 0, sizeof(name2));
  snprintf(name2, VERSIONSIZE, "version %i", VERSION);
  memcpy(save_p, name2, VERSIONSIZE);
  save_p += VERSIONSIZE;

  *save_p++ = static_cast<std::byte>(gameskill);
  *save_p++ = static_cast<std::byte>(gameepisode);
  *save_p++ = static_cast<std::byte>(gamemap);
  for (i = 0; i < MAXPLAYERS; i++)
    *save_p++ = static_cast<std::byte>(playeringame[i]);
  *save_p++ = static_cast<std::byte>(leveltime >> 16);
  *save_p++ = static_cast<std::byte>(leveltime >> 8);
  *save_p++ = static_cast<std::byte>(leveltime);

  P_ArchivePlayers();
  P_ArchiveWorld();
  P_ArchiveThinkers();
  P_ArchiveSpecials();

  *save_p++ = std::byte{0x1d}; // consistancy marker

  const auto length = save_p - savebuffer;
  if (length > SAVEGAMESIZE)
    logger::error("Savegame buffer overrun");
  // M_WriteFile (name, savebuffer, static_cast<int>(length));
  gameaction = ga_nothing;
  savedescription[0] = 0;

  players[consoleplayer].message = GGSAVED;

  // draw the pattern into the back screen
  R_FillBackScreen();
}

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.
//
skill_t d_skill;
int d_episode;
int d_map;

void G_DeferedInitNew(skill_t skill, int episode, int map) {
  d_skill = skill;
  d_episode = episode;
  d_map = map;
  gameaction = ga_newgame;
}

void G_DoNewGame(void) {
  demoplayback = false;
  netdemo = false;
  netgame = false;
  deathmatch = false;
  playeringame[1] = playeringame[2] = playeringame[3] = 0;
  respawnparm = false;
  fastparm = false;
  nomonsters = false;
  consoleplayer = 0;
  G_InitNew(d_skill, d_episode, d_map);
  gameaction = ga_nothing;
}

void G_InitNew(skill_t skill, int episode, int map) {
  int i;

  if (paused) {
    paused = false;
    S_ResumeSound();
  }

  if (skill > skill_t::sk_nightmare)
      skill = skill_t::sk_nightmare;

  // This was quite messy with SPECIAL and commented parts.
  // Supposedly hacks to make the latest edition work.
  // It might not work properly.
  if (episode < 1)
    episode = 1;

  if (gamemode == retail) {
    if (episode > 4)
      episode = 4;
  } else if (gamemode == shareware) {
    if (episode > 1)
      episode = 1; // only start episode 1 on shareware
  } else {
    if (episode > 3)
      episode = 3;
  }

  if (map < 1)
    map = 1;

  if ((map > 9) && (gamemode != commercial))
    map = 9;

  M_ClearRandom();

  if (skill == skill_t::sk_nightmare || respawnparm)
    respawnmonsters = true;
  else
    respawnmonsters = false;

  if (fastparm ||
      (skill == skill_t::sk_nightmare && gameskill != skill_t::sk_nightmare))
  {
    for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
      states[i].tics >>= 1;
    mobjinfo[MT_BRUISERSHOT].speed = 20 * FRACUNIT;
    mobjinfo[MT_HEADSHOT].speed = 20 * FRACUNIT;
    mobjinfo[MT_TROOPSHOT].speed = 20 * FRACUNIT;
  }
  else if (skill != skill_t::sk_nightmare && gameskill == skill_t::sk_nightmare)
  {
    for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
      states[i].tics <<= 1;
    mobjinfo[MT_BRUISERSHOT].speed = 15 * FRACUNIT;
    mobjinfo[MT_HEADSHOT].speed = 10 * FRACUNIT;
    mobjinfo[MT_TROOPSHOT].speed = 10 * FRACUNIT;
  }

  // force players to be initialized upon first level load
  for (i = 0; i < MAXPLAYERS; i++)
    players[i].playerstate = PST_REBORN;

  usergame = true; // will be set false if a demo
  paused = false;
  demoplayback = false;
  automapactive = false;
  viewactive = true;
  gameepisode = episode;
  gamemap = map;
  gameskill = skill;

  viewactive = true;

  // set the sky map for the episode
  if (gamemode == commercial) {
    skytexture = R_TextureNumForName("SKY3");
    if (gamemap < 12)
      skytexture = R_TextureNumForName("SKY1");
    else if (gamemap < 21)
      skytexture = R_TextureNumForName("SKY2");
  } else
    switch (episode) {
    case 1:
      skytexture = R_TextureNumForName("SKY1");
      break;
    case 2:
      skytexture = R_TextureNumForName("SKY2");
      break;
    case 3:
      skytexture = R_TextureNumForName("SKY3");
      break;
    case 4: // Special Edition sky
      skytexture = R_TextureNumForName("SKY4");
      break;
    }

  G_DoLoadLevel();
}

//
// DEMO RECORDING
//
#define DEMOMARKER 0x80

void G_ReadDemoTiccmd(ticcmd_t *cmd) {
  if (*demo_p == static_cast<std::byte>(DEMOMARKER)) {
    // end of demo data stream
    G_CheckDemoStatus();
    return;
  }
  cmd->forwardmove = ((signed char)*demo_p++);
  cmd->sidemove = ((signed char)*demo_p++);
  cmd->angleturn = ((unsigned char)*demo_p++) << 8;
  cmd->buttons = (buttoncode_t)*demo_p++;
}

void G_WriteDemoTiccmd(ticcmd_t *cmd) {
  if (gamekeydown['q']) // press q to end demo recording
    G_CheckDemoStatus();
  *demo_p++ = static_cast<std::byte>(cmd->forwardmove);
  *demo_p++ = static_cast<std::byte>(cmd->sidemove);
  *demo_p++ = static_cast<std::byte>((cmd->angleturn + 128) >> 8);
  *demo_p++ = static_cast<std::byte>(cmd->buttons);
  demo_p -= 4;
  if (demo_p > demoend - 16) {
    // no more space
    G_CheckDemoStatus();
    return;
  }

  G_ReadDemoTiccmd(cmd); // make SURE it is exactly the same
}

//
// G_RecordDemo
//
void G_RecordDemo(std::string_view name) {
  int i;
  int maxsize;

  usergame = false;
  demoname.replace_filename( name );
  demoname.replace_extension(".lmp" );
  maxsize = 0x20000;
  i = arguments::has("-maxdemo");
  if (i && i < arguments::count() - 1)
    maxsize = atoi(arguments::at(i).data() + 1) * 1024;
  demobuffer = static_cast<std::byte *>(malloc(maxsize));
  demoend = demobuffer + maxsize;

  demorecording = true;
}

void G_BeginRecording(void) {
  int i;

  demo_p = demobuffer;

  *demo_p++ = static_cast<std::byte>(VERSION);
  *demo_p++ = static_cast<std::byte>(gameskill);
  *demo_p++ = static_cast<std::byte>(gameepisode);
  *demo_p++ = static_cast<std::byte>(gamemap);
  *demo_p++ = static_cast<std::byte>(deathmatch);
  *demo_p++ = static_cast<std::byte>(respawnparm);
  *demo_p++ = static_cast<std::byte>(fastparm);
  *demo_p++ = static_cast<std::byte>(nomonsters);
  *demo_p++ = static_cast<std::byte>(consoleplayer);

  for (i = 0; i < MAXPLAYERS; i++)
    *demo_p++ = static_cast<std::byte>(playeringame[i]);
}

//
// G_PlayDemo
//

std::string defdemoname;

void G_DeferedPlayDemo(std::string_view name) {
  defdemoname = name;
  gameaction = ga_playdemo;
}

void G_DoPlayDemo(void) {
  skill_t skill;
  int i, episode, map;

  gameaction = ga_nothing;
  demobuffer = demo_p =
      static_cast<std::byte *>(wad::get(defdemoname.c_str()));
  if (*demo_p++ != std::byte{VERSION}) {
    fprintf(stderr, "Demo is from a different game version!\n");
    gameaction = ga_nothing;
    return;
  }

  skill = static_cast<skill_t>(*demo_p++);
  episode = static_cast<int>(*demo_p++);
  map = static_cast<int>(*demo_p++);
  deathmatch = static_cast<uint8_t>(*demo_p++);
  respawnparm = static_cast<bool>(*demo_p++);
  fastparm = static_cast<bool>(*demo_p++);
  nomonsters = static_cast<bool>(*demo_p++);
  consoleplayer = static_cast<bool>(*demo_p++);

  for (i = 0; i < MAXPLAYERS; i++)
    playeringame[i] = static_cast<bool>(*demo_p++);
  if (playeringame[1]) {
    netgame = true;
    netdemo = true;
  }

  // don't spend a lot of time in loadlevel
  precache = false;
  G_InitNew(skill, episode, map);
  precache = true;

  usergame = false;
  demoplayback = true;
}

//
// G_TimeDemo
//
void G_TimeDemo(std::string_view name) {
  nodrawers = arguments::has("-nodraw");
  noblit = arguments::has("-noblit");
  timingdemo = true;
  singletics = true;

  defdemoname = name;
  gameaction = ga_playdemo;
}

/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

bool G_CheckDemoStatus(void) {
  int endtime;

  if (timingdemo) {
    endtime = I_GetTime();
    logger::error("timed {} gametics in {} realtics", gametic, endtime - starttime);
  }

  if (demoplayback) {
    if (singledemo)
      quit();

    demoplayback = false;
    netdemo = false;
    netgame = false;
    deathmatch = false;
    playeringame[1] = playeringame[2] = playeringame[3] = 0;
    respawnparm = false;
    fastparm = false;
    nomonsters = false;
    consoleplayer = 0;
    D_AdvanceDemo();
    return true;
  }

  if (demorecording) {
    *demo_p++ = std::byte{DEMOMARKER};
    // M_WriteFile (demoname, demobuffer, static_cast<int>(demo_p -
    // demobuffer));
    free(demobuffer);
    demorecording = false;
    logger::error("Demo %s recorded", demoname.string());
  }

  return false;
}
