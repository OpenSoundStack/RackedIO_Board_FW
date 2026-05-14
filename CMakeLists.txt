cmake_minimum_required(VERSION 3.20.0)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(BOARD rackedio)

find_package(Zephyr)
project(rackedio_board_firmware)

target_sources(app PRIVATE
        src/main.cpp
        src/ihm/rgb.h
        src/ihm/rgb.cpp
)
