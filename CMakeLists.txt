# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps=obj")
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(FlexiAssistGlove)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/zephyr)

target_include_directories(app PRIVATE src/app/pressure_sensor)
target_include_directories(app PRIVATE src/lib/imu)
target_include_directories(app PRIVATE src/lib/pump_ctrl)
target_include_directories(app PRIVATE src/util/logging)

FILE(GLOB pressure_sensor src/app/pressure_sensor/*.c)
FILE(GLOB imu src/lib/imu/*.c)
FILE(GLOB dev src/lib/pump_ctrl/*.c)

target_sources(app PRIVATE ${pressure_sensor})
target_sources(app PRIVATE ${imu})
target_sources(app PRIVATE ${dev})
target_sources(app PRIVATE src/main.c)


