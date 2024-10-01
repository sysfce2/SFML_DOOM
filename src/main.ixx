//-----------------------------------------------------------------------------
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
//
// DESCRIPTION:
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------
module;

#include "sounds.h"

#include "s_sound.h"
#include "v_video.h"


#include "i_video.h"

#include "g_game.h"

#include "hu_stuff.h"
#include "st_stuff.h"

#include "r_main.h"
#include "r_draw.h"

#include <SFML/Window/Event.hpp>

#include <spdlog/spdlog.h>

#include <filesystem>

export module main;

import system;
import menu;
import wad;
import argv;
import am_map;
import net;
import finale;
import sound;
import strings;
import doomstat;
import setup;
import wipe;
import intermission;

// List of wad files
std::vector<std::string> wadfilenames;
void D_AddFile(std::string_view file) { wadfilenames.emplace_back(file); }

export bool devparm;     // started game with -devparm
export bool nomonsters;  // checkparm of -nomonsters
export bool respawnparm; // checkparm of -respawn
export bool fastparm;    // checkparm of -fast

bool drone;

export bool singletics = false; // debug flag to cancel adaptiveness

// extern int soundVolume;
// extern  int	sfxVolume;
// extern  int	musicVolume;
bool inhelpscreens;

// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
export skill_t startskill;
export int startepisode;
export int startmap;
export bool autostart;

export FILE *debugfile;

export bool advancedemo;

char wadfile[1024];            // primary wad file
char mapdir[1024];             // directory of development maps

//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//
std::array<std::unique_ptr<sf::Event>, MAXEVENTS> events;
export int eventhead;
export int eventtail;

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
export void D_PostEvent(const sf::Event &ev) {
  events[eventhead] = std::make_unique<sf::Event>(ev);
  eventhead = (++eventhead) & (MAXEVENTS - 1);
}

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
export void D_ProcessEvents(void) {
  // IF STORE DEMO, DO NOT ACCEPT INPUT
  if ((gamemode == commercial) && (W_CheckNumForName("map01") < 0))
    return;

  for (; eventtail != eventhead; eventtail = (++eventtail) & (MAXEVENTS - 1)) {
    auto &ev = events[eventtail];
    if (M_Responder(*ev))
      continue; // menu ate the event
    G_Responder(*ev);
  }
}

std::string pagename;

//
// D_PageDrawer
//
void D_PageDrawer(void) {
  V_DrawPatch(0, 0, 0,
              static_cast<patch_t *>(W_CacheLumpName(pagename)));
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
export gamestate_t wipegamestate = GS_DEMOSCREEN;
bool setsizeneeded;

void D_Display(void) {
  static bool viewactivestate = false;
  static bool menuactivestate = false;
  static bool inhelpscreensstate = false;
  static bool fullscreen = false;
  static gamestate_t oldgamestate = static_cast<gamestate_t>(-1);
  static int borderdrawcount;
  int nowtime;
  int tics;
  int wipestart;
  int y;
  bool done;
  bool wipe;
  bool redrawsbar;

  if (nodrawers)
    return; // for comparative timing / profiling

  redrawsbar = false;

  // change the view size if needed
  if (setsizeneeded) {
    R_ExecuteSetViewSize();
    oldgamestate = static_cast<gamestate_t>(-1); // force background redraw
    borderdrawcount = 3;
  }

  // save the current screen if about to wipe
  if (gamestate != wipegamestate) {
    wipe = true;
    wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
  } else
    wipe = false;

  if (gamestate == GS_LEVEL && gametic)
    HU_Erase();

  // do buffered drawing
  switch (gamestate) {
  case GS_LEVEL:
    if (!gametic)
      break;
    if (automapactive)
      AM_Drawer();
    if (wipe || (viewheight != 200 && fullscreen))
      redrawsbar = true;
    if (inhelpscreensstate && !inhelpscreens)
      redrawsbar = true; // just put away the help screen
    ST_Drawer(viewheight == 200, redrawsbar);
    fullscreen = viewheight == 200;
    break;

  case GS_INTERMISSION:
    WI_Drawer();
    break;

  case GS_FINALE:
    F_Drawer();
    break;

  case GS_DEMOSCREEN:
    D_PageDrawer();
    break;
  }

  // draw buffered stuff to screen
  I_UpdateNoBlit();

  // draw the view directly
  if (gamestate == GS_LEVEL && !automapactive && gametic)
    R_RenderPlayerView(&players[displayplayer]);

  if (gamestate == GS_LEVEL && gametic)
    HU_Drawer();

  // clean up border stuff
  if (gamestate != oldgamestate && gamestate != GS_LEVEL)
    I_SetPalette(static_cast<std::byte *>(W_CacheLumpName("PLAYPAL")));

  // see if the border needs to be initially drawn
  if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL) {
    viewactivestate = false; // view was not active
    R_FillBackScreen();      // draw the pattern into the back screen
  }

  // see if the border needs to be updated to the screen
  if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != 320) {
    if (menuactive || menuactivestate || !viewactivestate)
      borderdrawcount = 3;
    if (borderdrawcount) {
      R_DrawViewBorder(); // erase old menu stuff
      borderdrawcount--;
    }
  }

  menuactivestate = menuactive;
  viewactivestate = viewactive;
  inhelpscreensstate = inhelpscreens;
  oldgamestate = wipegamestate = gamestate;

  // draw pause pic
  if (paused) {
    if (automapactive)
      y = 4;
    else
      y = viewwindowy + 4;
    V_DrawPatchDirect(
        viewwindowx + (scaledviewwidth - 68) / 2, y, 0,
        static_cast<patch_t *>(W_CacheLumpName("M_PAUSE")));
  }

  // menus go directly to the screen
  M_Drawer();  // menu is drawn even on top of everything
  NetUpdate(); // send out any new accumulation

  // normal update
  if (!wipe) {
    I_FinishUpdate(); // page flip or blit buffer
    return;
  }

  // wipe update
  wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

  wipestart = I_GetTime() - 1;

  do {
    do {
      nowtime = I_GetTime();
      tics = nowtime - wipestart;
    } while (!tics);
    wipestart = nowtime;
    done = wipe_ScreenWipe(wipe_Melt, 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
    I_UpdateNoBlit();
    M_Drawer();       // menu is drawn even on top of wipes
    I_FinishUpdate(); // page flip or blit buffer
  } while (!done);
}

//
//  DEMO LOOP
//
int demosequence;
int pagetic;

//
// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
//
export void D_DoAdvanceDemo(void) {
  players[consoleplayer].playerstate = PST_LIVE; // not reborn
  advancedemo = false;
  usergame = false; // no save / end game here
  paused = false;
  gameaction = ga_nothing;

  if (gamemode == retail)
    demosequence = (demosequence + 1) % 7;
  else
    demosequence = (demosequence + 1) % 6;

  switch (demosequence) {
  case 0:
    if (gamemode == commercial)
      pagetic = 35 * 11;
    else
      pagetic = 170;
    gamestate = GS_DEMOSCREEN;
    pagename = "TITLEPIC";
    if (gamemode == commercial)
      S_StartMusic(mus_dm2ttl);
    else
      S_StartMusic(mus_intro);
    break;
  case 1:
    G_DeferedPlayDemo("demo1");
    break;
  case 2:
    pagetic = 200;
    gamestate = GS_DEMOSCREEN;
    pagename = "CREDIT";
    break;
  case 3:
    G_DeferedPlayDemo("demo2");
    break;
  case 4:
    gamestate = GS_DEMOSCREEN;
    if (gamemode == commercial) {
      pagetic = 35 * 11;
      pagename = "TITLEPIC";
      S_StartMusic(mus_dm2ttl);
    } else {
      pagetic = 200;

      if (gamemode == retail)
        pagename = "CREDIT";
      else
        pagename = "HELP2";
    }
    break;
  case 5:
    G_DeferedPlayDemo("demo3");
    break;
    // THE DEFINITIVE DOOM Special Edition demo
  case 6:
    G_DeferedPlayDemo("demo4");
    break;
  }
}

//
// I_StartFrame
//
void I_StartFrame(void) {
  // er?
}

//
// D-DoomLoop()
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//
export bool demorecording;

void D_DoomLoop(void) {
  if (demorecording)
    G_BeginRecording();

  if (M_CheckParm("-debugfile")) {
    char filename[20];
    snprintf(filename, 20, "debug%i.txt", consoleplayer);
    printf("debug output to: %s\n", filename);
    debugfile = fopen(filename, "w");
  }

  I_InitGraphics();

  while (1) {
    // frame syncronous IO operations
    I_StartFrame();

    // process one or more tics
    if (singletics) {
      I_StartTic();
      D_ProcessEvents();
      G_BuildTiccmd(&netcmds[consoleplayer][maketic % BACKUPTICS]);
      if (advancedemo)
        D_DoAdvanceDemo();
      M_Ticker();
      G_Ticker();
      gametic++;
      maketic++;
    } else {
      if (advancedemo)
        D_DoAdvanceDemo();
      D_ProcessEvents();
      TryRunTics(); // will run at least one tic
    }

    S_UpdateSounds(players[consoleplayer].mo); // move positional sounds

    // Update display, next frame, with current state.
    D_Display();

    // Sound mixing for the buffer is snychronous.
    I_UpdateSound();
  }
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
export void D_AdvanceDemo(void) { advancedemo = true; }

//
// D_PageTicker
// Handles timing for warped projection
//
export void D_PageTicker(void) {
  if (--pagetic < 0)
    D_AdvanceDemo();
}

//
// D_StartTitle
//
export void D_StartTitle(void) {
  gameaction = ga_nothing;
  demosequence = -1;
  D_AdvanceDemo();
}

//
// IdentifyVersion
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
void IdentifyVersion(void) {
  char *home;
  std::string waddir = "wads";
  auto doomwaddir = getenv("DOOMWADDIR");
  if (doomwaddir) {
    waddir = doomwaddir;
  }

  spdlog::info("WAD directory: {}", (std::filesystem::current_path() / waddir).string());

  // Commercial.
  const auto doom2wad = waddir + "/doom2.wad";

  // Retail.
  const auto doomuwad = waddir + "/doomu.wad";

  // Registered.
  const auto doomwad = waddir + "/doom.wad";

  // Shareware.
  const auto doom1wad = waddir + "/doom1.wad";

  // Plutonia.
  const auto plutoniawad = waddir + "/plutonia.wad";

  // TNT.
  const auto tntwad = waddir + "/tnt.wad";

  // French stuff.
  const auto doom2fwad = waddir + "/doom2f.wad";

  if (M_CheckParm("-shdev")) {
    gamemode = shareware;
    devparm = true;
    D_AddFile( "doom1.wad");
    D_AddFile( "data_se/texture1.lmp");
    D_AddFile( "data_se/pnames.lmp");
    return;
  }

  if (M_CheckParm("-regdev")) {
    gamemode = registered;
    devparm = true;
    D_AddFile( "doom.wad");
    D_AddFile( "data_se/texture1.lmp");
    D_AddFile( "data_se/texture2.lmp");
    D_AddFile( "data_se/pnames.lmp");
    return;
  }

  if (M_CheckParm("-comdev")) {
    gamemode = commercial;
    devparm = true;
    /* I don't bother
    if(plutonia)
        D_AddFile ("plutonia.wad");
    else if(tnt)
        D_AddFile ("tnt.wad");
    else*/
    D_AddFile( "doom2.wad");

    D_AddFile( "cdata/texture1.lmp");
    D_AddFile( "cdata/pnames.lmp");
    return;
  }

  if (std::filesystem::exists(doom2fwad)) {
    gamemode = commercial;
    // C'est ridicule!
    // Let's handle languages in config files, okay?
    language = french;
    printf("French version\n");
    D_AddFile(doom2fwad.c_str());
    return;
  }

  if (std::filesystem::exists(doom2wad)) {
    gamemode = commercial;
    D_AddFile(doom2wad.c_str());
    return;
  }

  if (std::filesystem::exists(plutoniawad)) {
    gamemode = commercial;
    D_AddFile(plutoniawad.c_str());
    return;
  }

  if (std::filesystem::exists(tntwad)) {
    gamemode = commercial;
    D_AddFile(tntwad.c_str());
    return;
  }

  if (std::filesystem::exists(doomuwad)) {
    gamemode = retail;
    D_AddFile(doomuwad.c_str());
    return;
  }

  if (std::filesystem::exists(doomwad)) {
    gamemode = registered;
    D_AddFile(doomwad.c_str());
    return;
  }

  if (std::filesystem::exists(doom1wad)) {
    gamemode = shareware;
    D_AddFile(doom1wad.c_str());
    return;
  }

  printf("Game mode indeterminate.\n");
  gamemode = indetermined;

  // We don't abort. Let's see what the PWAD contains.
  // exit(1);
  // I_Error ("Game mode indeterminate\n");
}

export fixed_t forwardmove[2] = {0x19, 0x32};
export fixed_t sidemove[2] = {0x18, 0x28};

//
// D_DoomMain()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls N_AdvanceDemo.
//
export void D_DoomMain(void) {
  int p;
  std::string file;

  IdentifyVersion();

  modifiedgame = false;

  nomonsters = M_CheckParm("-nomonsters");
  respawnparm = M_CheckParm("-respawn");
  fastparm = M_CheckParm("-fast");
  devparm = M_CheckParm("-devparm");
  if (M_CheckParm("-altdeath"))
    deathmatch = 2;
  else if (M_CheckParm("-deathmatch"))
    deathmatch = 1;

  switch (gamemode) {
  case retail:
    spdlog::info("The Ultimate DOOM Startup v{}.{}", VERSION / 100, VERSION % 100);
    break;
  case shareware:
    spdlog::info("DOOM Shareware Startup v{}.{}", VERSION / 100, VERSION % 100);
    break;
  case registered:
    spdlog::info("DOOM Registered Startup v{}.{}", VERSION / 100, VERSION % 100);
    break;
  case commercial:
    spdlog::info("DOOM 2: Hell on Earth v{}.{}", VERSION / 100, VERSION % 100);
    break;
    /*FIXME
           case pack_plut:
            sprintf (title,
                     "                   "
                     "DOOM 2: Plutonia Experiment v%i.%i"
                     "                           ",
                     VERSION/100,VERSION%100);
            break;
          case pack_tnt:
            sprintf (title,
                     "                     "
                     "DOOM 2: TNT - Evilution v%i.%i"
                     "                           ",
                     VERSION/100,VERSION%100);
            break;
    */
  default:
    spdlog::info("Public DOOM v{}.{}", VERSION / 100, VERSION % 100);
    break;
  }

  if (devparm)
    printf(D_DEVSTR);

  // turbo option
  if ((p = M_CheckParm("-turbo"))) {
    int scale = 200;

    if (p < myargc - 1)
      scale = atoi(myargv[p + 1].c_str());
    if (scale < 10)
      scale = 10;
    if (scale > 400)
      scale = 400;
    spdlog::info("turbo scale: {}%%\n", scale);
    forwardmove[0] = forwardmove[0] * scale / 100;
    forwardmove[1] = forwardmove[1] * scale / 100;
    sidemove[0] = sidemove[0] * scale / 100;
    sidemove[1] = sidemove[1] * scale / 100;
  }

  // add any files specified on the command line with -file wadfile
  // to the wad list
  //
  // convenience hack to allow -wart e m to add a wad file
  // prepend a tilde to the filename so wadfile will be reloadable
  p = M_CheckParm("-wart");
  if (p) {
    myargv[p][4] = 'p'; // big hack, change to -warp

    // Map name handling.
    switch (gamemode) {
    case shareware:
    case retail:
    case registered:
      //snprintf(file, 256, "~/E%cM%c.wad", myargv[p + 1][0],
        //       myargv[p + 2][0]);
      //printf("Warping to Episode %s, Map %s.\n", myargv[p + 1].c_str(),
        //     myargv[p + 2].c_str());
      break;

    case commercial:
    default:
      p = atoi(myargv[p + 1].c_str());
      if (p < 10){}
        //snprintf(file, 256, "~/cdata/map0%i.wad", p);
      else{}
        //snprintf(file, 256, "~/cdata/map%i.wad", p);
      break;
    }
    D_AddFile(file);
  }

  p = M_CheckParm("-file");
  if (p) {
    // the parms after p are wadfile/lump names,
    // until end of parms or another - preceded parm
    modifiedgame = true; // homebrew levels
    while (++p != myargc && myargv[p][0] != '-')
      D_AddFile(myargv[p].c_str());
  }

  p = M_CheckParm("-playdemo");

  if (!p)
    p = M_CheckParm("-timedemo");

  if (p && p < myargc - 1) {
    //snprintf(file, 256, "%s.lmp", myargv[p + 1].c_str());
    D_AddFile(file);
    printf("Playing demo %s.lmp.\n", myargv[p + 1].c_str());
  }

  // get skill / episode / map from parms
  startskill = sk_medium;
  startepisode = 1;
  startmap = 1;
  autostart = false;

  p = M_CheckParm("-skill");
  if (p && p < myargc - 1) {
    startskill = static_cast<skill_t>(myargv[p + 1][0] - '1');
    autostart = true;
  }

  p = M_CheckParm("-episode");
  if (p && p < myargc - 1) {
    startepisode = myargv[p + 1][0] - '0';
    startmap = 1;
    autostart = true;
  }

  p = M_CheckParm("-timer");
  if (p && p < myargc - 1 && deathmatch) {
    int time;
    time = atoi(myargv[p + 1].c_str());
    printf("Levels will end after %d minute", time);
    if (time > 1)
      printf("s");
    printf(".\n");
  }

  p = M_CheckParm("-avg");
  if (p && p < myargc - 1 && deathmatch)
    printf("Austin Virtual Gaming: Levels will end after 20 minutes\n");

  p = M_CheckParm("-warp");
  if (p && p < myargc - 1) {
    if (gamemode == commercial)
      startmap = atoi(myargv[p + 1].c_str());
    else {
      startepisode = myargv[p + 1][0] - '0';
      startmap = myargv[p + 2][0] - '0';
    }
    autostart = true;
  }

  // init subsystems
  M_LoadDefaults(); // load before initing other systems

  spdlog::info("Z_Init: Init zone memory allocation daemon.");

  spdlog::info("W_Init: Init WADfiles.");
  W_InitMultipleFiles(wadfilenames);

  // Check for -file in shareware
  if (modifiedgame) {
    // These are the lumps that will be checked in IWAD,
    // if any one is not present, execution will be aborted.
    std::array name = {"e2m1",   "e2m2",   "e2m3",    "e2m4",   "e2m5",
                       "e2m6",   "e2m7",   "e2m8",    "e2m9",   "e3m1",
                       "e3m3",   "e3m3",   "e3m4",    "e3m5",   "e3m6",
                       "e3m7",   "e3m8",   "e3m9",    "dphoof", "bfgga0",
                       "heada1", "cybra1", "spida1d1"};
    int i;

    if (gamemode == shareware)
      I_Error("\nYou cannot -file with the shareware "
              "version. Register!");

    // Check for fake IWAD with right name,
    // but w/o all the lumps of the registered version.
    if (gamemode == registered)
      for (i = 0; i < 23; i++)
        if (W_CheckNumForName(name[i]) < 0)
          I_Error("\nThis is not the registered version.");
  }

  // Iff additonal PWAD files are used, print modified banner
  if (modifiedgame) {
    /*m*/ printf(
        "======================================================================"
        "=====\n"
        "ATTENTION:  This version of DOOM has been modified.  If you would "
        "like to\n"
        "get a copy of the original game, call 1-800-IDGAMES or see the readme "
        "file.\n"
        "        You will not receive technical support for modified games.\n"
        "                      press enter to continue\n"
        "======================================================================"
        "=====\n");
    getchar();
  }

  // Check and print which version is executed.
  switch (gamemode) {
  case shareware:
  case indetermined:
    printf("==================================================================="
           "========\n"
           "                                Shareware!\n"
           "==================================================================="
           "========\n");
    break;
  case registered:
  case retail:
  case commercial:
    printf("==================================================================="
           "========\n"
           "                 Commercial product - do not distribute!\n"
           "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
           "==================================================================="
           "========\n");
    break;

  default:
    // Ouch.
    break;
  }

  M_Init();

  printf("R_Init: Init DOOM refresh daemon - ");
  R_Init();

  printf("\nP_Init: Init Playloop state.\n");
  P_Init();

  printf("I_Init: Setting up machine state.\n");
  I_Init();

  printf("D_CheckNetGame: Checking network game status.\n");
  D_CheckNetGame();

  printf("S_Init: Setting up sound.\n");
  S_Init(snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/);

  printf("HU_Init: Setting up heads up display.\n");
  HU_Init();

  printf("ST_Init: Init status bar.\n");
  ST_Init();

  // start the apropriate game based on parms
  p = M_CheckParm("-record");

  if (p && p < myargc - 1) {
    G_RecordDemo(myargv[p + 1]);
    autostart = true;
  }

  p = M_CheckParm("-playdemo");
  if (p && p < myargc - 1) {
    singledemo = true; // quit after one demo
    G_DeferedPlayDemo(myargv[p + 1]);
    D_DoomLoop(); // never returns
  }

  p = M_CheckParm("-timedemo");
  if (p && p < myargc - 1) {
    G_TimeDemo(myargv[p + 1]);
    D_DoomLoop(); // never returns
  }

  p = M_CheckParm("-loadgame");
  if (p && p < myargc - 1) {
    file = std::format("{}{}.dsg",SAVEGAMENAME,myargv[p+1][0]);
    G_LoadGame(file);
  }

  if (gameaction != ga_loadgame) {
    if (autostart || netgame)
      G_InitNew(startskill, startepisode, startmap);
    else
      D_StartTitle(); // start up intro loop
  }

  D_DoomLoop(); // never returns
}

// Main entry point, read arguments and call doom main
export int main(int argc, char **argv) {
  myargc = argc;
  while (*argv) {
    myargv.emplace_back(*(argv++));
  }

  D_DoomMain();

  return 0;
}

