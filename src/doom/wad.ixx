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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------
module;

#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

export module wad;

import engine;

namespace wad
{

//! Represents the header of a WAD file
struct header
{
    std::array<char, 4> identification; //!< WAD type, should be "IWAD" or "PWAD"
    int32_t numlumps;                   //!< Number of lumps in the wad
    int32_t infotableofs;               //!< Offset of the info table
};
static_assert( sizeof( header ) == 12 );

//
// WADFILE I/O related stuff.
//
struct lump_info
{
    int32_t position = {};    //!< Position of the lump in it's file
    int32_t size = {};        //!< Size of the lump
    std::array<char, 8> name; //!< Name of the lump
};

static_assert( sizeof( lump_info ) == 16 );

// Info about the loaded lumps
export std::vector<lump_info> lumpinfo;

// Cache of the loaded lump data
using lump = std::vector<std::byte>;
std::vector<lump> lumpcache;

size_t reloadlump;
std::filesystem::path reloadpath;


//! All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...
void add( const std::filesystem::path &filepath )
{
    if ( filepath.empty() )
    {
        logger::error( "Cannot add empty file path" );
        return;
    }

    if ( !std::filesystem::exists( filepath ) )
    {
        logger::error( "File not found: {}", filepath.string() );
        return;
    }

    auto filename = filepath.filename();

    // handle reload indicator.
    if ( filename.string()[0] == '~' )
    {
        filename = filename.string().substr( 1 );
        reloadpath = filepath;
        reloadlump = lumpinfo.size();
    }

    std::ifstream file( filepath.string(), std::ios::binary );
    if ( !file.is_open() )
    {
        logger::error( " couldn't open {}", filepath.string() );
        return;
    }

    logger::info( "Adding wad file from {}", filename.string() );

    if ( filename.extension() != ".wad" )
    {
        // single lump file, just add it to the file lump info
        lump_info lump{
            .position = 0,
            .size = static_cast<int32_t>( std::filesystem::file_size( filepath ) ),
        };
        std::copy_n( filename.string().begin(), 8, lump.name.begin() );
        lumpinfo.emplace_back( lump );
    }
    else
    {
        // WAD file containing multiple lumps
        header header;
        file.read( reinterpret_cast<char *>( &header ), sizeof( header ) );
        if ( std::ranges::equal( header.identification, "IWAD" ) )
        {
            if ( std::ranges::equal( header.identification, "PWAD" ) )
            {
                logger::error( "Wad file {} doesn't have or PWAD id", filename.string() );
            }
        }

        logger::info( "WAD has {} lumps at offset {}", header.numlumps, header.infotableofs );

        auto length = header.numlumps * sizeof( lump_info );
        lumpinfo.resize( header.numlumps );
        file.seekg( header.infotableofs, std::ios::beg );
        file.read( reinterpret_cast<char *>( lumpinfo.data() ), length );
    }

    // Then cache the actual lumps
    for ( const auto info : lumpinfo )
    {
        lumpcache.emplace_back( info.size );
        file.seekg( info.position, std::ios::beg);
        file.read( reinterpret_cast<char *>(lumpcache.back().data()), info.size );
    }
}

//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
export void W_Reload( void )
{
    if ( reloadpath.empty() )
        return;

    // JONNY TODO
    // if ( (handle = open (reloadname,O_RDONLY | O_BINARY)) == -1)
    // logger::error ("W_Reload: couldn't open %s",reloadname);

    // JONNY TODO
    // read (handle, &header, sizeof(header));
    // lumpcount = header.numlumps;
    // header.infotableofs = header.infotableofs;
    // length = lumpcount*sizeof(filelump_t);
    // fileinfo = alloca (length);
    // lseek (handle, header.infotableofs, SEEK_SET);
    // read (handle, fileinfo, length);

    // Fill in lumpinfo
    // lump_p = &lumpinfo[reloadlump];

    // for ( auto i = reloadlump; i < reloadlump + lumpcount; i++, lump_p++, fileinfo++ )
    //{
    //     if ( lumpcache[i] )
    //         free( lumpcache[i] );
    //
    //     lump_p->position = fileinfo->filepos;
    //     lump_p->size = fileinfo->size;
    // }

    // JONNY TODO
    // close (handle);
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
export void add( std::vector<std::string> &filenames )
{
    // Clear existing lump infop
    lumpinfo.clear();

    for ( const auto &name : filenames )
    {
        add( name );
    }

    if ( lumpinfo.empty() )
    {
        logger::error( "W_InitFiles: no files found" );
    }

    // set up caching
    lumpcache.resize( lumpinfo.size() );
}

//
// W_InitFile
// Just initialize from a single file.
//
void W_InitFile( std::string filename )
{
    std::vector names{ filename };
    wad::add( names );
}


//
// wad::index_of
// Calls W_CheckNumForName, but bombs out if not found.
//
export int index_of( const std::string &name )
{
    // name must be 8 characters and upper case
    std::string upper_name( 8, 0 );
    std::transform( std::begin( name ), std::end( name ), std::begin( upper_name ), toupper );

    // scan backwards so patch lump files take precedence
    for ( auto lump = lumpinfo.rbegin(); lump != lumpinfo.rend(); ++lump )
    {
        if ( std::ranges::equal( lump->name, upper_name ) )
        {
            return static_cast<int>( std::distance( lumpinfo.begin(), lump.base() ) - 1 );
        }
    }

    logger::warn( "wad::index_of: {} not found!", name );

    return -1;
}

//
// wad::lump_length
// Returns the buffer size needed to load the given lump.
//
export int lump_length( int lump )
{
    if ( lump >= lumpinfo.size() )
    {
        logger::error( "wad::lump_length: {} out of bounds", lump );
    }

    return lumpinfo[lump].size;
}

//! Get the data for a lump
export void* get( uint32_t lump )
{
    if ( lump >= lumpinfo.size() )
    {
        logger::error( "wad::get: {} out of bounds", lump );
    }

    return lumpcache[lump].data();
}

//! Get the data for a lump
export void* get( const std::string& name )
{
    const auto lump = wad::index_of( name ); 
    if ( lump >= lumpinfo.size() )
    {
        logger::error( "wad::get: {} out of bounds", lump );
    }

    return lumpcache[lump].data();
}
} // namespace wad