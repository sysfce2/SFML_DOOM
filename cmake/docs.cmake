find_package(Doxygen)

if (NOT DOXYGEN_FOUND)
	message(FATAL_ERROR "Doxygen not found")
	return()
endif()

# Just document engine files
file(GLOB FILES ${CMAKE_CURRENT_LIST_DIR}/../src/*[.ixx|.cpp])

set(DOXYGEN_NUM_PROC_THREADS 0) # Use as many threads as the host has
set(DOXYGEN_WARN_AS_ERROR YES) # Treat warnings as errors
set(DOXYGEN_WARN_NO_PARAMDOC YES) # Warn if missing parameter documentation
set(DOXYGEN_WARN_IF_UNDOC_ENUM_VAL YES) # Warn if enum value is undocumented
doxygen_add_docs(docs ${FILES})

# And open browser once done
add_custom_command(TARGET docs POST_BUILD COMMAND start $<SHELL_PATH:${CMAKE_CURRENT_BINARY_DIR}/html/index.html>)