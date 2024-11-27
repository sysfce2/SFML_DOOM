module;
#include "g_game.h"
#include "i_video.h"
#include "r_draw.h"
#include "r_main.h"

#include <string>
module app;

import doom;
import main;
import wad;
import engine;
import menu;
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

//
// IdentifyVersion
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
void IdentifyVersion( void )
{
    std::string waddir = "wads";

    logger::info( "WAD directory: {}",
                  ( std::filesystem::current_path() / waddir ).string() );

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

    if ( arguments::has( "-shdev" ) )
    {
        gamemode = shareware;
        wadfilenames.emplace_back( "doom1.wad" );
        wadfilenames.emplace_back( "data_se/texture1.lmp" );
        wadfilenames.emplace_back( "data_se/pnames.lmp" );
        return;
    }

    if ( arguments::has( "-regdev" ) )
    {
        gamemode = registered;
        wadfilenames.emplace_back( "doom.wad" );
        wadfilenames.emplace_back( "data_se/texture1.lmp" );
        wadfilenames.emplace_back( "data_se/texture2.lmp" );
        wadfilenames.emplace_back( "data_se/pnames.lmp" );
        return;
    }

    if ( arguments::has( "-comdev" ) )
    {
        gamemode = commercial;
        /* I don't bother
        if(plutonia)
            wadfilenames.emplace_back ("plutonia.wad");
        else if(tnt)
            wadfilenames.emplace_back ("tnt.wad");
        else*/
        wadfilenames.emplace_back( "doom2.wad" );

        wadfilenames.emplace_back( "cdata/texture1.lmp" );
        wadfilenames.emplace_back( "cdata/pnames.lmp" );
        return;
    }

    if ( std::filesystem::exists( doom2fwad ) )
    {
        gamemode = commercial;
        // C'est ridicule!
        // Let's handle languages in config files, okay?
        language = french;
        printf( "French version\n" );
        wadfilenames.emplace_back( doom2fwad.c_str() );
        return;
    }

    if ( std::filesystem::exists( doom2wad ) )
    {
        gamemode = commercial;
        wadfilenames.emplace_back( doom2wad.c_str() );
        return;
    }

    if ( std::filesystem::exists( plutoniawad ) )
    {
        gamemode = commercial;
        wadfilenames.emplace_back( plutoniawad.c_str() );
        return;
    }

    if ( std::filesystem::exists( tntwad ) )
    {
        gamemode = commercial;
        wadfilenames.emplace_back( tntwad.c_str() );
        return;
    }

    if ( std::filesystem::exists( doomuwad ) )
    {
        gamemode = retail;
        wadfilenames.emplace_back( doomuwad.c_str() );
        return;
    }

    if ( std::filesystem::exists( doomwad ) )
    {
        gamemode = registered;
        wadfilenames.emplace_back( doomwad.c_str() );
        return;
    }

    if ( std::filesystem::exists( doom1wad ) )
    {
        gamemode = shareware;
        wadfilenames.emplace_back( doom1wad.c_str() );
        return;
    }

    printf( "Game mode indeterminate.\n" );
    gamemode = indetermined;

    // We don't abort. Let's see what the PWAD contains.
    // exit(1);
    // logger::error ("Game mode indeterminate\n");
}

void init( void )
{
    int p;
    std::string file;

    IdentifyVersion();

    modifiedgame = false;

    nomonsters = arguments::has( "-nomonsters" );
    respawnparm = arguments::has( "-respawn" );
    fastparm = arguments::has( "-fast" );
    if ( arguments::has( "-altdeath" ) )
        deathmatch = 2;
    else if ( arguments::has( "-deathmatch" ) )
        deathmatch = 1;

    switch ( gamemode )
    {
    case retail:
        logger::info( "The Ultimate DOOM Startup v{}.{}", VERSION / 100,
                      VERSION % 100 );
        break;
    case shareware:
        logger::info( "DOOM Shareware Startup v{}.{}", VERSION / 100,
                      VERSION % 100 );
        break;
    case registered:
        logger::info( "DOOM Registered Startup v{}.{}", VERSION / 100,
                      VERSION % 100 );
        break;
    case commercial:
        logger::info( "DOOM 2: Hell on Earth v{}.{}", VERSION / 100,
                      VERSION % 100 );
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
        logger::info( "Public DOOM v{}.{}", VERSION / 100, VERSION % 100 );
        break;
    }

#if !NDEBUG
    printf( D_DEVSTR );
#endif

    // turbo option
    if ( ( p = arguments::has( "-turbo" ) ) )
    {
        int scale = 200;

        if ( p < arguments::count() - 1 )
            scale = atoi( arguments::at( p + 1 ).data() );
        if ( scale < 10 )
            scale = 10;
        if ( scale > 400 )
            scale = 400;
        logger::info( "turbo scale: {}%%\n", scale );
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
    p = arguments::has( "-wart" );
    if ( p )
    {
        // @TODO JONNY
        // arguments::at(p)[4] = 'p'; // big hack, change to -warp

        // Map name handling.
        switch ( gamemode )
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
            p = atoi( arguments::at( p + 1 ).data() );
            if ( p < 10 )
            {
            }
            // snprintf(file, 256, "~/cdata/map0%i.wad", p);
            else
            {
            }
            // snprintf(file, 256, "~/cdata/map%i.wad", p);
            break;
        }
        wadfilenames.emplace_back( file );
    }

    p = arguments::has( "-file" );
    if ( p )
    {
        // the parms after p are wadfile/lump names,
        // until end of parms or another - preceded parm
        modifiedgame = true; // homebrew levels
        while ( ++p != arguments::count() && arguments::at( p )[0] != '-' )
            wadfilenames.emplace_back( arguments::at( p ).data() );
    }

    p = arguments::has( "-playdemo" );

    if ( !p )
        p = arguments::has( "-timedemo" );

    if ( p && p < arguments::count() - 1 )
    {
        // snprintf(file, 256, "%s.lmp", arguments::at(p + 1).c_str());
        wadfilenames.emplace_back( file );
        printf( "Playing demo %s.lmp.\n", arguments::at( p + 1 ).data() );
    }

    // get skill / episode / map from parms
    startskill = skill_t::sk_medium;
    startepisode = 1;
    startmap = 1;
    autostart = false;

    p = arguments::has( "-skill" );
    if ( p && p < arguments::count() - 1 )
    {
        startskill = static_cast<skill_t>( arguments::at( p + 1 )[0] - '1' );
        autostart = true;
    }

    p = arguments::has( "-episode" );
    if ( p && p < arguments::count() - 1 )
    {
        startepisode = arguments::at( p + 1 )[0] - '0';
        startmap = 1;
        autostart = true;
    }

    p = arguments::has( "-timer" );
    if ( p && p < arguments::count() - 1 && deathmatch )
    {
        int time;
        time = atoi( arguments::at( p + 1 ).data() );
        printf( "Levels will end after %d minute", time );
        if ( time > 1 )
            printf( "s" );
        printf( ".\n" );
    }

    p = arguments::has( "-avg" );
    if ( p && p < arguments::count() - 1 && deathmatch )
        printf( "Austin Virtual Gaming: Levels will end after 20 minutes\n" );

    p = arguments::has( "-warp" );
    if ( p && p < arguments::count() - 1 )
    {
        if ( gamemode == commercial )
            startmap = atoi( arguments::at( p + 1 ).data() );
        else
        {
            startepisode = arguments::at( p + 1 )[0] - '0';
            startmap = arguments::at( p + 2 )[0] - '0';
        }
        autostart = true;
    }

    // init subsystems
    M_LoadDefaults(); // load before initing other systems

    logger::info( "W_Init: Init WADfiles." );
    W_InitMultipleFiles( wadfilenames );

    // Check for -file in shareware
    if ( modifiedgame )
    {
        // These are the lumps that will be checked in IWAD,
        // if any one is not present, execution will be aborted.
        std::array name = { "e2m1",   "e2m2",   "e2m3",    "e2m4",   "e2m5",
                            "e2m6",   "e2m7",   "e2m8",    "e2m9",   "e3m1",
                            "e3m3",   "e3m3",   "e3m4",    "e3m5",   "e3m6",
                            "e3m7",   "e3m8",   "e3m9",    "dphoof", "bfgga0",
                            "heada1", "cybra1", "spida1d1" };
        int i;

        if ( gamemode == shareware )
            logger::error( "\nYou cannot -file with the shareware "
                           "version. Register!" );

        // Check for fake IWAD with right name,
        // but w/o all the lumps of the registered version.
        if ( gamemode == registered )
            for ( i = 0; i < 23; i++ )
                if ( W_CheckNumForName( name[i] ) < 0 )
                    logger::error( "\nThis is not the registered version." );
    }

    // Iff additonal PWAD files are used, print modified banner
    if ( modifiedgame )
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
            "=====\n" );
        getchar();
    }

    // Check and print which version is executed.
    switch ( gamemode )
    {
    case shareware:
    case indetermined:
        printf(
            "==============================================================="
            "===="
            "========\n"
            "                                Shareware!\n"
            "==============================================================="
            "===="
            "========\n" );
        break;
    case registered:
    case retail:
    case commercial:
        printf(
            "==============================================================="
            "===="
            "========\n"
            "                 Commercial product - do not distribute!\n"
            "         Please report software piracy to the SPA: "
            "1-800-388-PIR8\n"
            "==============================================================="
            "===="
            "========\n" );
        break;

    default:
        // Ouch.
        break;
    }

    M_Init();

    printf( "R_Init: Init DOOM refresh daemon - " );
    R_Init();

    printf( "\nP_Init: Init Playloop state.\n" );
    P_Init();

    I_InitSound();

    printf( "I_Init: Setting up machine state.\n" );
    I_Init();

    printf( "D_CheckNetGame: Checking network game status.\n" );
    D_CheckNetGame();

    printf( "S_Init: Setting up sound.\n" );
    S_Init( snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/ );

    printf( "HU_Init: Setting up heads up display.\n" );
    HU_Init();

    printf( "ST_Init: Init status bar.\n" );
    ST_Init();

    // start the apropriate game based on parms
    p = arguments::has( "-record" );

    if ( p && p < arguments::count() - 1 )
    {
        G_RecordDemo( arguments::at( p + 1 ) );
        autostart = true;
    }

    p = arguments::has( "-playdemo" );
    if ( p && p < arguments::count() - 1 )
    {
        singledemo = true; // quit after one demo
        G_DeferedPlayDemo( arguments::at( p + 1 ) );
        return;
    }

    p = arguments::has( "-timedemo" );
    if ( p && p < arguments::count() - 1 )
    {
        G_TimeDemo( arguments::at( p + 1 ) );
        return;
    }

    p = arguments::has( "-loadgame" );
    if ( p && p < arguments::count() - 1 )
    {
        file =
            std::format( "{}{}.dsg", SAVEGAMENAME, arguments::at( p + 1 )[0] );
        G_LoadGame( file );
    }

    if ( gameaction != gameaction_t::ga_loadgame )
    {
        if ( autostart || netgame )
            G_InitNew( startskill, startepisode, startmap );
        else
            D_StartTitle(); // start up intro loop
    }

    if ( demorecording )
        G_BeginRecording();

    I_InitGraphics();
}

void update()
{
    // process one or more tics
    if ( singletics )
    {
        I_StartTic();
        D_ProcessEvents();
        G_BuildTiccmd( &netcmds[consoleplayer][maketic % BACKUPTICS] );
        if ( advancedemo )
            D_DoAdvanceDemo();
        M_Ticker();
        G_Ticker();
        gametic++;
        maketic++;
    }
    else
    {
        if ( advancedemo )
            D_DoAdvanceDemo();
        D_ProcessEvents();
        TryRunTics(); // will run at least one tic
    }

    S_UpdateSounds( players[consoleplayer].mo ); // move positional sounds

    // Update display, next frame, with current state.
    D_Display();
}
