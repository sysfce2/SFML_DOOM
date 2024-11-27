export module app;

import engine;

//! Initialise the application
//! 
//! This is left undefined by the engine and must be implemented by the app
export void init();

//! Main entry point
//! @param argc Argument count
//! @param argv Argument values
export int main(int argc, char **argv)
{
    arguments::parse(argc, argv);

    init();

    return 0;
}
