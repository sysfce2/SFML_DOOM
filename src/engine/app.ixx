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
    arguments::parse( argc, argv );

    init();

    while ( true )
    {
        update();
    }

    return 0;
}
