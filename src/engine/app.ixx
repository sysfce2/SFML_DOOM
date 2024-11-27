export module app;

import engine;

export void init();

// Main entry point, read arguments and call doom main
export int main(int argc, char **argv)
{
    arguments::parse(argc, argv);

    init();

    return 0;
}
