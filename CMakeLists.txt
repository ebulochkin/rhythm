cmake_minimum_required(VERSION 3.16)
project(rhythm_game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(
        rhythm main.cpp
        chart.h
        chart.cpp
        audio.h
        audio.cpp
        )

target_link_libraries(rhythm raylib)
