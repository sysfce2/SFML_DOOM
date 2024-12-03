module;
#include "spdlog/spdlog.h"
export module app;

import engine;

//! Initialise the application
//!
//! This is left undefined by the engine and must be implemented by the app
export void init();

//! Run one update of the app
//!
//! This is left undefined by the engine and must be implemented by the app
export void update();

bool quitting = {}; //!< Set to true when quitting

//! Quit the application
export void quit() { quitting = true; }

//! Main entry point
//! @param argc Argument count
//! @param argv Argument values
export int main( int argc, char **argv )
{
#if !NDEBUG
    spdlog::set_level( spdlog::level::debug );
#endif

    arguments::parse( argc, argv );

    logger::info( "Initialising app" );
    init();

    logger::info( "Initialisation done, starting update loop" );
    while ( !quitting )
    {
        update();
    }

    return 0;
}

//! Get the current tick count
//! @return the number of ticks (1/70s) since the app started
export int get_current_tick()
{
    using namespace std::chrono;
    using tic = duration<int, std::ratio<1, 70>>;
    const auto now = steady_clock::now();
    static const auto basetime = now;
    const auto tics = duration_cast<tic>( now - basetime ).count();
    return tics;
}