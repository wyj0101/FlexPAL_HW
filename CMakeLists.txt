# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps=obj")
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(FlexiAssistGlove)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/zephyr)

target_include_directories(app PRIVATE 
    src/app/sensor
    src/app/uart_handle
    src/app/esp_wifi
    src/lib/imu
    src/lib/pressure_sensor
    src/lib/pump_ctrl
    src/lib/flash_rw
    src/util/logging
    src/util
)


FILE(GLOB app src/app/*/*.c)
FILE(GLOB lib src/lib/*/*.c)

target_sources(app PRIVATE ${app})
target_sources(app PRIVATE ${lib})
target_sources(app PRIVATE src/main.c)


