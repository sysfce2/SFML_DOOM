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
    while ( true )
    {
        update();
    }

    return 0;
}