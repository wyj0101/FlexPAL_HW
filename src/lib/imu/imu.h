#pragma once

#define IMU_STACK_SIZE 768

union udp_value {
	float value;
	uint8_t bytes[4];
};
typedef struct imu_data {
	union udp_value acce_x;
	union udp_value acce_y;
	union udp_value acce_z;
	union udp_value gyro_x;
	union udp_value gyro_y;
	union udp_value gyro_z;
	union udp_value temp;
} imu_data;

int get_imu_value(imu_data *value);
int imu_init(void);