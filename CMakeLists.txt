# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps=obj")
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(FlexiAssistGlove)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/zephyr)

target_include_directories(app PRIVATE src/app/sensor)
target_include_directories(app PRIVATE src/app/imu)
target_include_directories(app PRIVATE src/app/dev)

FILE(GLOB sensor src/app/sensor/*.c)
FILE(GLOB imu src/app/imu/*.c)
FILE(GLOB dev src/app/dev/*.c)

target_sources(app PRIVATE ${sensor})
target_sources(app PRIVATE ${imu})
target_sources(app PRIVATE ${dev})
target_sources(app PRIVATE src/main.c)


