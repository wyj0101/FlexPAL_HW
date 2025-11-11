#pragma once

#define PRESSURE_SENSOR_STACK_SIZE 512

#define SPI_FREQUENCY 1250000 // 32分频，1.125M
#define SPI_OPERTION (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))


void pressure_sensor_init();