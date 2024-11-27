project(hello)

add_executable(${PROJECT_NAME} hello.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC ${CMAKE_PROJECT_NAME})