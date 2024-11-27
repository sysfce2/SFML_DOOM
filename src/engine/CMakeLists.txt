add_library(${CMAKE_PROJECT_NAME})

# c++ modules source
file(GLOB_RECURSE MODULE_SOURCE CONFIGURE_DEPENDS *.ixx)
target_sources(${CMAKE_PROJECT_NAME} PUBLIC FILE_SET CXX_MODULES FILES ${MODULE_SOURCE})

# We use c++23
target_compile_features(${CMAKE_PROJECT_NAME} PUBLIC cxx_std_23)

# We want to treat compiler warnings as errors
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY COMPILE_WARNING_AS_ERROR YES)

# Use fetch content to fetch dependencies if they are not found
include(FetchContent)

# SFML we take from master - can pin to an SFML 3 release when there is one
# We use this for window, audio and graphics
set(SFML_STATIC_LIBRARIES ON)
set(SFML_FIND_COMPONENTS Graphics Audio)
FetchContent_Declare(
        sfml
        GIT_REPOSITORY    https://github.com/sfml/sfml
        GIT_TAG           master
        FIND_PACKAGE_ARGS
)

# spdlog we use for logging
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog
        GIT_TAG v1.14.1
        FIND_PACKAGE_ARGS
)

# Make them available
FetchContent_MakeAvailable(sfml spdlog)

# Link them (should be PRIVATE once refactored)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC SFML::Graphics SFML::Audio spdlog::spdlog)