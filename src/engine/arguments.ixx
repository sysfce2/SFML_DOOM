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
// DESCRIPTION: Handles command line arguments
//
//-----------------------------------------------------------------------------
module;
#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

export module engine:arguments;

namespace arguments
{
export void parse(int argc, char **argv);
export bool has(std::string_view argument);
export uint8_t index_of(std::string_view argument);
export std::string_view at(uint8_t index);
export uint8_t count();

std::vector<std::string> arguments;

void parse(int argc, char **argv)
{
    for (auto i : std::views::iota(0, argc))
    {
        arguments.emplace_back(argv[i]);
    }
}

bool has(std::string_view argument)
{
    return std::ranges::contains(arguments, argument);
}

uint8_t index_of(std::string_view argument)
{
    return std::distance(arguments.begin(),
                         std::ranges::find(arguments, argument));
}

std::string_view at(uint8_t index) { return arguments[index]; }

uint8_t count() { return arguments.size(); }
} // namespace arguments