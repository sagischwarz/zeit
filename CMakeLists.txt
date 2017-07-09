cmake_minimum_required(VERSION 3.7)
project(zeit)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES src/main.c)
add_executable(zeit ${SOURCE_FILES})
target_link_libraries(zeit readline sqlite3)
