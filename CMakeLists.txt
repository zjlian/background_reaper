cmake_minimum_required(VERSION 3.0.0)
project(bg_reaper VERSION 0.1.0)

add_executable(test test.cpp)

target_link_libraries(test PUBLIC pthread)