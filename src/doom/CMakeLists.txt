project(doom)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE)

# c++ modules source
file(GLOB_RECURSE MODULE_SOURCE CONFIGURE_DEPENDS *.ixx)
target_sources(${PROJECT_NAME} PUBLIC FILE_SET CXX_MODULES FILES ${MODULE_SOURCE})

# Legacy cpp files
file(GLOB SOURCE CONFIGURE_DEPENDS ../../linuxdoom-1.10/*.c ../../linuxdoom-1.10/*.cpp ../../linuxdoom-1.10/*.h *.cpp)
target_sources(${PROJECT_NAME} PUBLIC ${SOURCE})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ../../linuxdoom-1.10)

# Sym link the wads folder into the build output
if(APPLE)
    set(WADS_DEST $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/MacOS/wads)
else()
    set(WADS_DEST $<TARGET_FILE_DIR:${PROJECT_NAME}>/wads)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/wads ${WADS_DEST})

# VS startup project should be sfml-doom
set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# VS working directory should be the output directory
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# Enable the legacy range checks, can probably be disabled once modernised
target_compile_definitions(${PROJECT_NAME} PRIVATE RANGE_CHECK)

# Depends on engine
target_link_libraries(${PROJECT_NAME} PUBLIC ${CMAKE_PROJECT_NAME})