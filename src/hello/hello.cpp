module app;

void init()
{
    logger::debug( "Hello in debug builds!" );
    logger::info( "Hello!" );
    logger::warn( "Hello warning!" );
    logger::error( "Hello-h no!" );
}

void update() { quit(); }