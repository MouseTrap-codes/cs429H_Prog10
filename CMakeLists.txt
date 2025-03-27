cmake_minimum_required(VERSION 3.16)
project(project3)

set(CMAKE_C_STANDARD 99)

add_subdirectory(libtdmm)

add_executable(project3 main.c)
target_link_libraries(project3 tdmm)