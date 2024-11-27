find_program(CLANG_FORMAT clang-format)

if(CLANG_FORMAT)
	message(STATUS "clang-format found: ${CLANG_FORMAT}")
else()
	message(STATUS "clang-format not found")
	return()
endif()


file(GLOB_RECURSE FILES ${CMAKE_CURRENT_LIST_DIR}/../src/*[.ixx|.cpp])
add_custom_target(format
	COMMAND ${CLANG_FORMAT} -style=file ${FILES} -i
	COMMENT "Formatting code"
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..)