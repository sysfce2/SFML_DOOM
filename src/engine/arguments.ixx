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
//-----------------------------------------------------------------------------
module;
#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

export module engine:arguments;

import :logger;

namespace arguments
{

using namespace std::literals;

std::vector<std::string> arguments;

//! Parse command line arguments
//! @param argc Argument count
//! @param argv Argument values
export void parse( int argc, char **argv )
{
    auto message = "Parsed arguments: "s;
    for ( auto i : std::views::iota( 0, argc ) )
    {
        arguments.emplace_back( argv[i] );
        message += argv[i];
    }

    logger::info( "{}", message );
}

//! Check if we were given a command line argument
//! @param argument Argument value to check for
//! @return True if the argument was present
export bool has( std::string_view argument ) { return std::ranges::contains( arguments, argument ); }

//! Get the index of an argument from the command line
//! @param argument Argument value to get the index of
//! @return The index of the given argument
export uint8_t index_of( std::string_view argument ) { return std::distance( arguments.begin(), std::ranges::find( arguments, argument ) ); }

//! Get the argument at a given index
//! @param index The index to get the argument at
//! @return The argument value at the given index
export std::string_view at( uint8_t index ) { return arguments[index]; }

//! Get the total count of arguments
//! @return the total count of arguments
export uint8_t count() { return arguments.size(); }
} // namespace arguments