cmake_minimum_required(VERSION 3.28)
project(engine)

# Add targets
add_subdirectory(src/engine)
add_subdirectory(src/hello)
add_subdirectory(src/doom)

# Target to format code
add_custom_target(format ${CMAKE_COMMAND} -P cmake/format.cmake)
