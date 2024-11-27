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
#include "g_game.h"
#include "i_video.h"
#include "r_draw.h"
#include "r_main.h"
#include <SFML/Window/Event.hpp>
#include <filesystem>
#include <spdlog/spdlog.h>
export module main;

import engine;
import menu;
import wad;
import engine;
import doom.map;
import net;
import finale;
import sound;
import doom;
import setup;
import wipe;
import intermission;
import video;
import status_bar;
import hud;
import sound;

// List of wad files we populate on start up
std::vector<std::string> wadfilenames;

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

export bool advancedemo;

char wadfile[1024]; // primary wad file
char mapdir[1024];  // directory of development maps

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
export void D_PostEvent(const sf::Event &ev)
{
    events[eventhead] = std::make_unique<sf::Event>(ev);
    eventhead = (++eventhead) & (MAXEVENTS - 1);
}

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
export void D_ProcessEvents(void)
{
    // IF STORE DEMO, DO NOT ACCEPT INPUT
    if ((gamemode == commercial) && (W_CheckNumForName("map01") < 0))
        return;

    for (; eventtail != eventhead; eventtail = (++eventtail) & (MAXEVENTS - 1))
    {
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
void D_PageDrawer(void)
{
    V_DrawPatch(0, 0, 0, static_cast<patch_t *>(W_CacheLumpName(pagename)));
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
export gamestate_t wipegamestate = GS_DEMOSCREEN;
export bool setsizeneeded;

void D_Display(void)
{
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
    if (setsizeneeded)
    {
        R_ExecuteSetViewSize();
        oldgamestate = static_cast<gamestate_t>(-1); // force background redraw
        borderdrawcount = 3;
    }

    // save the current screen if about to wipe
    if (gamestate != wipegamestate)
    {
        wipe = true;
        wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
    }
    else
        wipe = false;

    if (gamestate == GS_LEVEL && gametic)
        HU_Erase();

    // do buffered drawing
    switch (gamestate)
    {
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
    if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL)
    {
        viewactivestate = false; // view was not active
        R_FillBackScreen();      // draw the pattern into the back screen
    }

    // see if the border needs to be updated to the screen
    if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != 320)
    {
        if (menuactive || menuactivestate || !viewactivestate)
            borderdrawcount = 3;
        if (borderdrawcount)
        {
            R_DrawViewBorder(); // erase old menu stuff
            borderdrawcount--;
        }
    }

    menuactivestate = menuactive;
    viewactivestate = viewactive;
    inhelpscreensstate = inhelpscreens;
    oldgamestate = wipegamestate = gamestate;

    // draw pause pic
    if (paused)
    {
        if (automapactive)
            y = 4;
        else
            y = viewwindowy + 4;
        V_DrawPatchDirect(viewwindowx + (scaledviewwidth - 68) / 2, y, 0,
                          static_cast<patch_t *>(W_CacheLumpName("M_PAUSE")));
    }

    // menus go directly to the screen
    M_Drawer();  // menu is drawn even on top of everything
    NetUpdate(); // send out any new accumulation

    // normal update
    if (!wipe)
    {
        I_FinishUpdate(); // page flip or blit buffer
        return;
    }

    // wipe update
    wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

    wipestart = I_GetTime() - 1;

    do
    {
        do
        {
            nowtime = I_GetTime();
            tics = nowtime - wipestart;
        } while (!tics);
        wipestart = nowtime;
        done =
            wipe_ScreenWipe(wipe_Melt, 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
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
export void D_DoAdvanceDemo(void)
{
    players[consoleplayer].playerstate = PST_LIVE; // not reborn
    advancedemo = false;
    usergame = false; // no save / end game here
    paused = false;
    gameaction = gameaction_t::ga_nothing;

    if (gamemode == retail)
        demosequence = (demosequence + 1) % 7;
    else
        demosequence = (demosequence + 1) % 6;

    switch (demosequence)
    {
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
        if (gamemode == commercial)
        {
            pagetic = 35 * 11;
            pagename = "TITLEPIC";
            S_StartMusic(mus_dm2ttl);
        }
        else
        {
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
void I_StartFrame(void)
{
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

void D_DoomLoop(void)
{
    if (demorecording)
        G_BeginRecording();

    I_InitGraphics();

    while (1)
    {
        // frame syncronous IO operations
        I_StartFrame();

        // process one or more tics
        if (singletics)
        {
            I_StartTic();
            D_ProcessEvents();
            G_BuildTiccmd(&netcmds[consoleplayer][maketic % BACKUPTICS]);
            if (advancedemo)
                D_DoAdvanceDemo();
            M_Ticker();
            G_Ticker();
            gametic++;
            maketic++;
        }
        else
        {
            if (advancedemo)
                D_DoAdvanceDemo();
            D_ProcessEvents();
            TryRunTics(); // will run at least one tic
        }

        S_UpdateSounds(players[consoleplayer].mo); // move positional sounds

        // Update display, next frame, with current state.
        D_Display();
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
export void D_PageTicker(void)
{
    if (--pagetic < 0)
        D_AdvanceDemo();
}

//
// D_StartTitle
//
export void D_StartTitle(void)
{
    gameaction = gameaction_t::ga_nothing;
    demosequence = -1;
    D_AdvanceDemo();
}

//
// IdentifyVersion
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
void IdentifyVersion(void)
{
    std::string waddir = "wads";

    spdlog::info("WAD directory: {}",
                 (std::filesystem::current_path() / waddir).string());

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

    if (arguments::has("-shdev"))
    {
        gamemode = shareware;
        wadfilenames.emplace_back("doom1.wad");
        wadfilenames.emplace_back("data_se/texture1.lmp");
        wadfilenames.emplace_back("data_se/pnames.lmp");
        return;
    }

    if (arguments::has("-regdev"))
    {
        gamemode = registered;
        wadfilenames.emplace_back("doom.wad");
        wadfilenames.emplace_back("data_se/texture1.lmp");
        wadfilenames.emplace_back("data_se/texture2.lmp");
        wadfilenames.emplace_back("data_se/pnames.lmp");
        return;
    }

    if (arguments::has("-comdev"))
    {
        gamemode = commercial;
        /* I don't bother
        if(plutonia)
            wadfilenames.emplace_back ("plutonia.wad");
        else if(tnt)
            wadfilenames.emplace_back ("tnt.wad");
        else*/
        wadfilenames.emplace_back("doom2.wad");

        wadfilenames.emplace_back("cdata/texture1.lmp");
        wadfilenames.emplace_back("cdata/pnames.lmp");
        return;
    }

    if (std::filesystem::exists(doom2fwad))
    {
        gamemode = commercial;
        // C'est ridicule!
        // Let's handle languages in config files, okay?
        language = french;
        printf("French version\n");
        wadfilenames.emplace_back(doom2fwad.c_str());
        return;
    }

    if (std::filesystem::exists(doom2wad))
    {
        gamemode = commercial;
        wadfilenames.emplace_back(doom2wad.c_str());
        return;
    }

    if (std::filesystem::exists(plutoniawad))
    {
        gamemode = commercial;
        wadfilenames.emplace_back(plutoniawad.c_str());
        return;
    }

    if (std::filesystem::exists(tntwad))
    {
        gamemode = commercial;
        wadfilenames.emplace_back(tntwad.c_str());
        return;
    }

    if (std::filesystem::exists(doomuwad))
    {
        gamemode = retail;
        wadfilenames.emplace_back(doomuwad.c_str());
        return;
    }

    if (std::filesystem::exists(doomwad))
    {
        gamemode = registered;
        wadfilenames.emplace_back(doomwad.c_str());
        return;
    }

    if (std::filesystem::exists(doom1wad))
    {
        gamemode = shareware;
        wadfilenames.emplace_back(doom1wad.c_str());
        return;
    }

    printf("Game mode indeterminate.\n");
    gamemode = indetermined;

    // We don't abort. Let's see what the PWAD contains.
    // exit(1);
    // logger::error ("Game mode indeterminate\n");
}

export fixed_t forwardmove[2] = {0x19, 0x32};
export fixed_t sidemove[2] = {0x18, 0x28};

//
// D_DoomMain()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls N_AdvanceDemo.
//
export void D_DoomMain(void)
{
    int p;
    std::string file;

    IdentifyVersion();

    modifiedgame = false;

    nomonsters = arguments::has("-nomonsters");
    respawnparm = arguments::has("-respawn");
    fastparm = arguments::has("-fast");
    if (arguments::has("-altdeath"))
        deathmatch = 2;
    else if (arguments::has("-deathmatch"))
        deathmatch = 1;

    switch (gamemode)
    {
    case retail:
        spdlog::info("The Ultimate DOOM Startup v{}.{}", VERSION / 100,
                     VERSION % 100);
        break;
    case shareware:
        spdlog::info("DOOM Shareware Startup v{}.{}", VERSION / 100,
                     VERSION % 100);
        break;
    case registered:
        spdlog::info("DOOM Registered Startup v{}.{}", VERSION / 100,
                     VERSION % 100);
        break;
    case commercial:
        spdlog::info("DOOM 2: Hell on Earth v{}.{}", VERSION / 100,
                     VERSION % 100);
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

#if !NDEBUG
    printf(D_DEVSTR);
#endif

    // turbo option
    if ((p = arguments::has("-turbo")))
    {
        int scale = 200;

        if (p < arguments::count() - 1)
            scale = atoi(arguments::at(p + 1).data());
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
    p = arguments::has("-wart");
    if (p)
    {
        // @TODO JONNY
        // arguments::at(p)[4] = 'p'; // big hack, change to -warp

        // Map name handling.
        switch (gamemode)
        {
        case shareware:
        case retail:
        case registered:
            // snprintf(file, 256, "~/E%cM%c.wad", arguments::at(p + 1)[0],
            //        arguments::at(p + 2)[0]);
            // printf("Warping to Episode %s, Map %s.\n", arguments::at(p +
            // 1).c_str(),
            //      arguments::at(p + 2).c_str());
            break;

        case commercial:
        default:
            p = atoi(arguments::at(p + 1).data());
            if (p < 10)
            {
            }
            // snprintf(file, 256, "~/cdata/map0%i.wad", p);
            else
            {
            }
            // snprintf(file, 256, "~/cdata/map%i.wad", p);
            break;
        }
        wadfilenames.emplace_back(file);
    }

    p = arguments::has("-file");
    if (p)
    {
        // the parms after p are wadfile/lump names,
        // until end of parms or another - preceded parm
        modifiedgame = true; // homebrew levels
        while (++p != arguments::count() && arguments::at(p)[0] != '-')
            wadfilenames.emplace_back(arguments::at(p).data());
    }

    p = arguments::has("-playdemo");

    if (!p)
        p = arguments::has("-timedemo");

    if (p && p < arguments::count() - 1)
    {
        // snprintf(file, 256, "%s.lmp", arguments::at(p + 1).c_str());
        wadfilenames.emplace_back(file);
        printf("Playing demo %s.lmp.\n", arguments::at(p + 1).data());
    }

    // get skill / episode / map from parms
    startskill = skill_t::sk_medium;
    startepisode = 1;
    startmap = 1;
    autostart = false;

    p = arguments::has("-skill");
    if (p && p < arguments::count() - 1)
    {
        startskill = static_cast<skill_t>(arguments::at(p + 1)[0] - '1');
        autostart = true;
    }

    p = arguments::has("-episode");
    if (p && p < arguments::count() - 1)
    {
        startepisode = arguments::at(p + 1)[0] - '0';
        startmap = 1;
        autostart = true;
    }

    p = arguments::has("-timer");
    if (p && p < arguments::count() - 1 && deathmatch)
    {
        int time;
        time = atoi(arguments::at(p + 1).data());
        printf("Levels will end after %d minute", time);
        if (time > 1)
            printf("s");
        printf(".\n");
    }

    p = arguments::has("-avg");
    if (p && p < arguments::count() - 1 && deathmatch)
        printf("Austin Virtual Gaming: Levels will end after 20 minutes\n");

    p = arguments::has("-warp");
    if (p && p < arguments::count() - 1)
    {
        if (gamemode == commercial)
            startmap = atoi(arguments::at(p + 1).data());
        else
        {
            startepisode = arguments::at(p + 1)[0] - '0';
            startmap = arguments::at(p + 2)[0] - '0';
        }
        autostart = true;
    }

    // init subsystems
    M_LoadDefaults(); // load before initing other systems

    spdlog::info("W_Init: Init WADfiles.");
    W_InitMultipleFiles(wadfilenames);

    // Check for -file in shareware
    if (modifiedgame)
    {
        // These are the lumps that will be checked in IWAD,
        // if any one is not present, execution will be aborted.
        std::array name = {"e2m1",   "e2m2",   "e2m3",    "e2m4",   "e2m5",
                           "e2m6",   "e2m7",   "e2m8",    "e2m9",   "e3m1",
                           "e3m3",   "e3m3",   "e3m4",    "e3m5",   "e3m6",
                           "e3m7",   "e3m8",   "e3m9",    "dphoof", "bfgga0",
                           "heada1", "cybra1", "spida1d1"};
        int i;

        if (gamemode == shareware)
            logger::error("\nYou cannot -file with the shareware "
                          "version. Register!");

        // Check for fake IWAD with right name,
        // but w/o all the lumps of the registered version.
        if (gamemode == registered)
            for (i = 0; i < 23; i++)
                if (W_CheckNumForName(name[i]) < 0)
                    logger::error("\nThis is not the registered version.");
    }

    // Iff additonal PWAD files are used, print modified banner
    if (modifiedgame)
    {
        /*m*/ printf(
            "=================================================================="
            "===="
            "=====\n"
            "ATTENTION:  This version of DOOM has been modified.  If you would "
            "like to\n"
            "get a copy of the original game, call 1-800-IDGAMES or see the "
            "readme "
            "file.\n"
            "        You will not receive technical support for modified "
            "games.\n"
            "                      press enter to continue\n"
            "=================================================================="
            "===="
            "=====\n");
        getchar();
    }

    // Check and print which version is executed.
    switch (gamemode)
    {
    case shareware:
    case indetermined:
        printf("==============================================================="
               "===="
               "========\n"
               "                                Shareware!\n"
               "==============================================================="
               "===="
               "========\n");
        break;
    case registered:
    case retail:
    case commercial:
        printf("==============================================================="
               "===="
               "========\n"
               "                 Commercial product - do not distribute!\n"
               "         Please report software piracy to the SPA: "
               "1-800-388-PIR8\n"
               "==============================================================="
               "===="
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

    I_InitSound();

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
    p = arguments::has("-record");

    if (p && p < arguments::count() - 1)
    {
        G_RecordDemo(arguments::at(p + 1));
        autostart = true;
    }

    p = arguments::has("-playdemo");
    if (p && p < arguments::count() - 1)
    {
        singledemo = true; // quit after one demo
        G_DeferedPlayDemo(arguments::at(p + 1));
        D_DoomLoop(); // never returns
    }

    p = arguments::has("-timedemo");
    if (p && p < arguments::count() - 1)
    {
        G_TimeDemo(arguments::at(p + 1));
        D_DoomLoop(); // never returns
    }

    p = arguments::has("-loadgame");
    if (p && p < arguments::count() - 1)
    {
        file = std::format("{}{}.dsg", SAVEGAMENAME, arguments::at(p + 1)[0]);
        G_LoadGame(file);
    }

    if (gameaction != gameaction_t::ga_loadgame)
    {
        if (autostart || netgame)
            G_InitNew(startskill, startepisode, startmap);
        else
            D_StartTitle(); // start up intro loop
    }

    D_DoomLoop(); // never returns
}

// Main entry point, read arguments and call doom main
export int main(int argc, char **argv)
{
    arguments::parse(argc, argv);

    D_DoomMain();

    return 0;
}
