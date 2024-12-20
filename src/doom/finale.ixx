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
//	Game completion, final screen animation.
//
//-----------------------------------------------------------------------------
module;
#include <ctype.h>

#include "g_game.h"
#include "p_pspr.h"
#include "r_state.h"
#include "p_mobj.h"

#include <string>

export module finale;

import menu;
import wad;
import doom.map;
import doom;
import video;
import sound;
import hud;
import info;

// ?
// //
// #include "f_finale.h"

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
int finalestage;

int finalecount;

#define TEXTSPEED 3
#define TEXTWAIT 250

auto e1text = E1TEXT;
auto e2text = E2TEXT;
auto e3text = E3TEXT;
auto e4text = E4TEXT;

auto c1text = C1TEXT;
auto c2text = C2TEXT;
auto c3text = C3TEXT;
auto c4text = C4TEXT;
auto c5text = C5TEXT;
auto c6text = C6TEXT;

auto p1text = P1TEXT;
auto p2text = P2TEXT;
auto p3text = P3TEXT;
auto p4text = P4TEXT;
auto p5text = P5TEXT;
auto p6text = P6TEXT;

auto t1text = T1TEXT;
auto t2text = T2TEXT;
auto t3text = T3TEXT;
auto t4text = T4TEXT;
auto t5text = T5TEXT;
auto t6text = T6TEXT;

std::string finaletext;
std::string finaleflat;

void F_StartCast( void );

void F_CastTicker( void );

bool F_CastResponder( const sf::Event &ev );

void F_CastDrawer( void );

//
// F_StartFinale
//
export void F_StartFinale( void )
{
    gameaction = gameaction_t::ga_nothing;
    gamestate = GS_FINALE;
    viewactive = false;
    automapactive = false;

    // Okay - IWAD dependend stuff.
    // This has been changed severly, and
    //  some stuff might have changed in the process.
    switch ( gamemode )
    {

    // DOOM 1 - E1, E3 or E4, but each nine missions
    case shareware:
    case registered:
    case retail:
    {
        S_ChangeMusic( mus_victor, true );

        switch ( gameepisode )
        {
        case 1:
            finaleflat = "FLOOR4_8";
            finaletext = e1text;
            break;
        case 2:
            finaleflat = "SFLR6_1";
            finaletext = e2text;
            break;
        case 3:
            finaleflat = "MFLR8_4";
            finaletext = e3text;
            break;
        case 4:
            finaleflat = "MFLR8_3";
            finaletext = e4text;
            break;
        default:
            // Ouch.
            break;
        }
        break;
    }

        // DOOM II and missions packs with E1, M34
    case commercial:
    {
        S_ChangeMusic( mus_read_m, true );

        switch ( gamemap )
        {
        case 6:
            finaleflat = "SLIME16";
            finaletext = c1text;
            break;
        case 11:
            finaleflat = "RROCK14";
            finaletext = c2text;
            break;
        case 20:
            finaleflat = "RROCK07";
            finaletext = c3text;
            break;
        case 30:
            finaleflat = "RROCK17";
            finaletext = c4text;
            break;
        case 15:
            finaleflat = "RROCK13";
            finaletext = c5text;
            break;
        case 31:
            finaleflat = "RROCK19";
            finaletext = c6text;
            break;
        default:
            // Ouch.
            break;
        }
        break;
    }

        // Indeterminate.
    default:
        S_ChangeMusic( mus_read_m, true );
        finaleflat = "F_SKY1"; // Not used anywhere else.
        finaletext = c1text;   // FIXME - other text, music?
        break;
    }

    finalestage = 0;
    finalecount = 0;
}

export bool F_Responder( const sf::Event &event )
{
    if ( finalestage == 2 )
        return F_CastResponder( event );

    return false;
}

//
// F_Ticker
//
export void F_Ticker( void )
{
    int i;

    // check for skipping
    if ( ( gamemode == commercial ) && ( finalecount > 50 ) )
    {
        // go on to the next level
        for ( i = 0; i < MAXPLAYERS; i++ )
            if ( players[i].cmd.buttons )
                break;

        if ( i < MAXPLAYERS )
        {
            if ( gamemap == 30 )
                F_StartCast();
            else
                gameaction = gameaction_t::ga_worlddone;
        }
    }

    // advance animation
    finalecount++;

    if ( finalestage == 2 )
    {
        F_CastTicker();
        return;
    }

    if ( gamemode == commercial )
        return;

    if ( !finalestage && finalecount > finaletext.length() * TEXTSPEED + TEXTWAIT )
    {
        finalecount = 0;
        finalestage = 1;
        // TODO JONNY Circular dep
        // wipegamestate = static_cast<gamestate_t>(-1); // force a wipe
        if ( gameepisode == 3 )
            S_StartMusic( mus_bunny );
    }
}

//
// F_TextWrite
//

void F_TextWrite( void )
{
    std::byte *src;
    std::byte *dest;

    int x, y, w;
    int count;
    const char *ch;
    int c;
    int cx;
    int cy;

    // erase the entire screen to a tiled background
    src = static_cast<std::byte *>( wad::get( finaleflat ) );
    dest = screens[0].data();

    for ( y = 0; y < SCREENHEIGHT; y++ )
    {
        for ( x = 0; x < SCREENWIDTH / 64; x++ )
        {
            memcpy( dest, src + ( ( y & 63 ) << 6 ), 64 );
            dest += 64;
        }
        if ( SCREENWIDTH & 63 )
        {
            memcpy( dest, src + ( ( y & 63 ) << 6 ), SCREENWIDTH & 63 );
            dest += ( SCREENWIDTH & 63 );
        }
    }

    V_MarkRect( 0, 0, SCREENWIDTH, SCREENHEIGHT );

    // draw some of the text onto the screen
    cx = 10;
    cy = 10;
    ch = finaletext.c_str();

    count = ( finalecount - 10 ) / TEXTSPEED;
    if ( count < 0 )
        count = 0;
    for ( ; count; count-- )
    {
        c = *ch++;
        if ( !c )
            break;
        if ( c == '\n' )
        {
            cx = 10;
            cy += 11;
            continue;
        }

        c = toupper( c ) - HU_FONTSTART;
        if ( c < 0 || c > HU_FONTSIZE )
        {
            cx += 4;
            continue;
        }

        w = hu_font[c]->width;
        if ( cx + w > SCREENWIDTH )
            break;
        V_DrawPatch( cx, cy, 0, hu_font[c] );
        cx += w;
    }
}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
typedef struct
{
    const char *name;
    mobjtype_t type;
} castinfo_t;

castinfo_t castorder[] = { { CC_ZOMBIE, MT_POSSESSED },
                           { CC_SHOTGUN, MT_SHOTGUY },
                           { CC_HEAVY, MT_CHAINGUY },
                           { CC_IMP, MT_TROOP },
                           { CC_DEMON, MT_SERGEANT },
                           { CC_LOST, MT_SKULL },
                           { CC_CACO, MT_HEAD },
                           { CC_HELL, MT_KNIGHT },
                           { CC_BARON, MT_BRUISER },
                           { CC_ARACH, MT_BABY },
                           { CC_PAIN, MT_PAIN },
                           { CC_REVEN, MT_UNDEAD },
                           { CC_MANCU, MT_FATSO },
                           { CC_ARCH, MT_VILE },
                           { CC_SPIDER, MT_SPIDER },
                           { CC_CYBER, MT_CYBORG },
                           { CC_HERO, MT_PLAYER },

                           { NULL, static_cast<mobjtype_t>( 0 ) } };

int castnum;
int casttics;
state_t *caststate;
bool castdeath;
int castframes;
int castonmelee;
bool castattacking;

//
// F_StartCast
//
void F_StartCast( void )
{
    // TODO JONNY Circular dep
    // wipegamestate = static_cast<gamestate_t>(-1); // force a screen wipe
    castnum = 0;
    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    casttics = static_cast<int>( caststate->tics );
    castdeath = false;
    finalestage = 2;
    castframes = 0;
    castonmelee = 0;
    castattacking = false;
    S_ChangeMusic( mus_evil, true );
}

//
// F_CastTicker
//
void F_CastTicker( void )
{
    int st;
    int sfx;

    if ( --casttics > 0 )
        return; // not time to change state yet

    if ( caststate->tics == -1 || caststate->nextstate == S_NULL )
    {
        // switch from deathstate to next monster
        castnum++;
        castdeath = false;
        if ( castorder[castnum].name == NULL )
            castnum = 0;
        if ( mobjinfo[castorder[castnum].type].seesound )
            S_StartSound( NULL, mobjinfo[castorder[castnum].type].seesound );
        caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        castframes = 0;
    }
    else
    {
        // just advance to next state in animation
        if ( caststate == &states[S_PLAY_ATK1] )
            goto stopattack; // Oh, gross hack!
        st = caststate->nextstate;
        caststate = &states[st];
        castframes++;

        // sound hacks....
        switch ( st )
        {
        case S_PLAY_ATK1:
            sfx = sfx_dshtgn;
            break;
        case S_POSS_ATK2:
            sfx = sfx_pistol;
            break;
        case S_SPOS_ATK2:
            sfx = sfx_shotgn;
            break;
        case S_VILE_ATK2:
            sfx = sfx_vilatk;
            break;
        case S_SKEL_FIST2:
            sfx = sfx_skeswg;
            break;
        case S_SKEL_FIST4:
            sfx = sfx_skepch;
            break;
        case S_SKEL_MISS2:
            sfx = sfx_skeatk;
            break;
        case S_FATT_ATK8:
        case S_FATT_ATK5:
        case S_FATT_ATK2:
            sfx = sfx_firsht;
            break;
        case S_CPOS_ATK2:
        case S_CPOS_ATK3:
        case S_CPOS_ATK4:
            sfx = sfx_shotgn;
            break;
        case S_TROO_ATK3:
            sfx = sfx_claw;
            break;
        case S_SARG_ATK2:
            sfx = sfx_sgtatk;
            break;
        case S_BOSS_ATK2:
        case S_BOS2_ATK2:
        case S_HEAD_ATK2:
            sfx = sfx_firsht;
            break;
        case S_SKULL_ATK2:
            sfx = sfx_sklatk;
            break;
        case S_SPID_ATK2:
        case S_SPID_ATK3:
            sfx = sfx_shotgn;
            break;
        case S_BSPI_ATK2:
            sfx = sfx_plasma;
            break;
        case S_CYBER_ATK2:
        case S_CYBER_ATK4:
        case S_CYBER_ATK6:
            sfx = sfx_rlaunc;
            break;
        case S_PAIN_ATK3:
            sfx = sfx_sklatk;
            break;
        default:
            sfx = 0;
            break;
        }

        if ( sfx )
            S_StartSound( NULL, sfx );
    }

    if ( castframes == 12 )
    {
        // go into attack frame
        castattacking = true;
        if ( castonmelee )
            caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
        else
            caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
        castonmelee ^= 1;
        if ( caststate == &states[S_NULL] )
        {
            if ( castonmelee )
                caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
            else
                caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
        }
    }

    if ( castattacking )
    {
        if ( castframes == 24 || caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
        {
        stopattack:
            castattacking = false;
            castframes = 0;
            caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        }
    }

    casttics = static_cast<int>( caststate->tics );
    if ( casttics == -1 )
        casttics = 15;
}

//
// F_CastResponder
//

bool F_CastResponder( const sf::Event &ev )
{
    if ( ev.is<sf::Event::KeyPressed>() )
        return false;

    if ( castdeath )
        return true; // already in dying frames

    // go into death frame
    castdeath = true;
    caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
    casttics = static_cast<int>( caststate->tics );
    castframes = 0;
    castattacking = false;
    if ( mobjinfo[castorder[castnum].type].deathsound )
        S_StartSound( NULL, mobjinfo[castorder[castnum].type].deathsound );

    return true;
}

void F_CastPrint( const char *text )
{
    const char *ch;
    int c;
    int cx;
    int w;
    int width;

    // find width
    ch = text;
    width = 0;

    while ( ch )
    {
        c = *ch++;
        if ( !c )
            break;
        c = toupper( c ) - HU_FONTSTART;
        if ( c < 0 || c > HU_FONTSIZE )
        {
            width += 4;
            continue;
        }

        w = hu_font[c]->width;
        width += w;
    }

    // draw it
    cx = 160 - width / 2;
    ch = text;
    while ( ch )
    {
        c = *ch++;
        if ( !c )
            break;
        c = toupper( c ) - HU_FONTSTART;
        if ( c < 0 || c > HU_FONTSIZE )
        {
            cx += 4;
            continue;
        }

        w = hu_font[c]->width;
        V_DrawPatch( cx, 180, 0, hu_font[c] );
        cx += w;
    }
}

//
// F_CastDrawer
//

void F_CastDrawer( void )
{
    int lump;
    bool flip;
    patch_t *patch;

    // erase the entire screen to a background
    V_DrawPatch( 0, 0, 0, static_cast<patch_t *>( wad::get( "BOSSBACK" ) ) );

    F_CastPrint( castorder[castnum].name );

    // draw the current frame in the middle of the screen
    const auto &sprdef = sprites[caststate->sprite];
    const auto &sprframe = sprdef[caststate->frame & FF_FRAMEMASK];
    lump = sprframe.lump[0];
    flip = (bool)sprframe.flip[0];

    patch = static_cast<patch_t *>( wad::get( lump + firstspritelump ) );
    if ( flip )
    {
    }
    // TODO JONNY circular dep
    // V_DrawPatchFlipped(160, 170, 0, patch);
    else
        V_DrawPatch( 160, 170, 0, patch );
}

//
// F_DrawPatchCol
//
void F_DrawPatchCol( int x, patch_t *patch, int col )
{
    column_t *column;
    std::byte *source;
    std::byte *dest;
    std::byte *desttop;
    int count;

    column = (column_t *)( (std::byte *)patch + patch->columnofs[col] );
    desttop = screens[0].data() + x;

    // step through the posts in a column
    while ( column->topdelta != std::byte{ 0xff } )
    {
        source = (std::byte *)column + 3;
        dest = desttop + static_cast<int>( column->topdelta ) * SCREENWIDTH;
        count = static_cast<int>( column->length );

        while ( count-- )
        {
            *dest = *source++;
            dest += SCREENWIDTH;
        }
        column = (column_t *)( (std::byte *)column + static_cast<int>( column->length ) + 4 );
    }
}

//
// F_BunnyScroll
//
void F_BunnyScroll( void )
{
    int scrolled;
    int x;
    patch_t *p1;
    patch_t *p2;
    char name[10];
    int stage;
    static int laststage;

    p1 = static_cast<patch_t *>( wad::get( "PFUB2" ) );
    p2 = static_cast<patch_t *>( wad::get( "PFUB1" ) );

    V_MarkRect( 0, 0, SCREENWIDTH, SCREENHEIGHT );

    scrolled = 320 - ( finalecount - 230 ) / 2;
    if ( scrolled > 320 )
        scrolled = 320;
    if ( scrolled < 0 )
        scrolled = 0;

    for ( x = 0; x < SCREENWIDTH; x++ )
    {
        if ( x + scrolled < 320 )
            F_DrawPatchCol( x, p1, x + scrolled );
        else
            F_DrawPatchCol( x, p2, x + scrolled - 320 );
    }

    if ( finalecount < 1130 )
        return;
    if ( finalecount < 1180 )
    {
        V_DrawPatch( ( SCREENWIDTH - 13 * 8 ) / 2, ( SCREENHEIGHT - 8 * 8 ) / 2, 0, static_cast<patch_t *>( wad::get( "END0" ) ) );
        laststage = 0;
        return;
    }

    stage = ( finalecount - 1180 ) / 5;
    if ( stage > 6 )
        stage = 6;
    if ( stage > laststage )
    {
        S_StartSound( NULL, sfx_pistol );
        laststage = stage;
    }

    snprintf( name, 10, "END%i", stage );
    V_DrawPatch( ( SCREENWIDTH - 13 * 8 ) / 2, ( SCREENHEIGHT - 8 * 8 ) / 2, 0, static_cast<patch_t *>( wad::get( name ) ) );
}

//
// F_Drawer
//
export void F_Drawer( void )
{
    if ( finalestage == 2 )
    {
        F_CastDrawer();
        return;
    }

    if ( !finalestage )
        F_TextWrite();
    else
    {
        switch ( gameepisode )
        {
        case 1:
            if ( gamemode == retail )
                V_DrawPatch( 0, 0, 0, static_cast<patch_t *>( wad::get( "CREDIT" ) ) );
            else
                V_DrawPatch( 0, 0, 0, static_cast<patch_t *>( wad::get( "HELP2" ) ) );
            break;
        case 2:
            V_DrawPatch( 0, 0, 0, static_cast<patch_t *>( wad::get( "VICTORY2" ) ) );
            break;
        case 3:
            F_BunnyScroll();
            break;
        case 4:
            V_DrawPatch( 0, 0, 0, static_cast<patch_t *>( wad::get( "ENDPIC" ) ) );
            break;
        }
    }
}
