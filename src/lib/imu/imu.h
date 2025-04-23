#pragma once

#define IMU_STACK_SIZE 768

typedef struct imu_data {
	float acce_x;
	float acce_y;
	float acce_z;
	float gyro_x;
	float gyro_y;
	float gyro_z;
	float temp;
} imu_data;

int get_imu_value(imu_data *value);