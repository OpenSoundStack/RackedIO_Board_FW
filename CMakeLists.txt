cmake_minimum_required(VERSION 3.20.0)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(BOARD rackedio)

find_package(Zephyr)
project(rackedio_board_firmware)

set(EMBEDDED_BUILD ON)
set(NO_THREADS ON)
add_subdirectory(OpenAudioNetwork)

target_link_libraries(oancommon PUBLIC zephyr_interface)
target_link_libraries(oannetutils PUBLIC zephyr_interface)

target_sources(app PRIVATE
        src/main.cpp
        src/ihm/rgb.h
        src/ihm/rgb.cpp
        src/net/EthernetRouter.cpp
        src/net/EthernetRouter.h
        src/net/EthFiltering.c
        src/net/EthFiltering.h
        src/net/RingBuffer.cpp
        src/net/RingBuffer.h
        src/net/SockSvc.c
)

target_link_libraries(app PRIVATE oancommon oannetutils)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(app PRIVATE NO_THREADS)
