#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include "imu.h"

static const struct device *imu_dev = DEVICE_DT_GET(DT_NODELABEL(imu));

int get_imu_value(imu_data *value)
{
	struct sensor_value temperature;
	struct sensor_value accel[3];
	struct sensor_value gyro[3];

	int rc = sensor_sample_fetch(imu_dev);

	if (rc == 0) {
		rc = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	}
	if (rc == 0) {
		rc = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro);
	}
	if (rc == 0) {
		rc = sensor_channel_get(imu_dev, SENSOR_CHAN_DIE_TEMP, &temperature);
	}
	if (rc == 0) {
		value->acce_x.value = sensor_value_to_float(&accel[0]);
		value->acce_y.value = sensor_value_to_float(&accel[1]);
		value->acce_z.value = sensor_value_to_float(&accel[2]);
		value->gyro_x.value = sensor_value_to_float(&gyro[0]);
		value->gyro_y.value = sensor_value_to_float(&gyro[1]);
		value->gyro_z.value = sensor_value_to_float(&gyro[2]);
		value->temp.value = sensor_value_to_float(&temperature);
	} else {
		printf("sample fetch/get failed: %d\n", rc);
		return rc;
	}
	return 0;
}