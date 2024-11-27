find_program(CLANG_FORMAT clang-format)

if(CLANG_FORMAT)
	message(STATUS "clang-format found: ${CLANG_FORMAT}")
else()
	message(FATAL_ERROR "clang-format not found")
endif()


file(GLOB_RECURSE FILES ${CMAKE_CURRENT_LIST_DIR}/../src/*[.ixx|.cpp])
execute_process(
	COMMAND ${CLANG_FORMAT} -style=file ${FILES} -i
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_FILE}/..
	COMMAND_ECHO STDOUT)