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
#include <spdlog/spdlog.h>

// For debug break on error
#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#endif
export module engine:logger;

namespace logger
{
//! Log an error message
//! @param fmt Format string to use
//! @param args Arguments for the format string
//! @warning This will exit the app and break in the debugger, if attached
export template <typename... Args> void error( spdlog::format_string_t<Args...> fmt, Args &&...args )
{
    spdlog::error( fmt, std::forward<Args>( args )... );

#if WIN32
    DebugBreak();
#else
    raise( SIGTRAP );
#endif

    exit( -1 );
}

//! Log an error message
//! @param fmt Format string to use
//! @param args Arguments for the format string
//! @warning This will exit the app and break in the debugger, if attached
export template <typename... Args> void warn( spdlog::format_string_t<Args...> fmt, Args &&...args )
{
    spdlog::warn( fmt, std::forward<Args>( args )... );
}

//! Log an info message
//! @param fmt Format string to use
//! @param args Arguments for the format string
export template <typename... Args> void info( spdlog::format_string_t<Args...> fmt, Args &&...args ) { spdlog::info( fmt, std::forward<Args>( args )... ); }

//! Log a debug message
//! @param fmt Format string to use
//! @param args Arguments for the format string
export template <typename... Args> void debug( spdlog::format_string_t<Args...> fmt, Args &&...args ) { spdlog::debug( fmt, std::forward<Args>( args )... ); }
} // namespace logger