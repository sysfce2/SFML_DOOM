cmake_minimum_required(VERSION 3.28)
project(engine)

# Add targets
add_subdirectory(src/engine)
add_subdirectory(src/hello)
add_subdirectory(src/doom)

# extra cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(docs)
include(format)
