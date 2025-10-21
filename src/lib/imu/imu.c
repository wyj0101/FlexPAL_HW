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
		value->acce_x.value = sensor_value_to_float(&accel[0]);
		value->acce_y.value = sensor_value_to_float(&accel[1]);
		value->acce_z.value = sensor_value_to_float(&accel[2]);
		value->gyro_x.value = sensor_value_to_float(&gyro[0]);
		value->gyro_y.value = sensor_value_to_float(&gyro[1]);
		value->gyro_z.value = sensor_value_to_float(&gyro[2]);
	} else {
		printf("sample fetch/get failed: %d\n", rc);
		return rc;
	}
	return 0;
}
int imu_init(void)
{
	struct sensor_value full_scale, sampling_freq, oversampling;

	if (!device_is_ready(imu_dev)) {
		printf("Device %s is not ready\n", imu_dev->name);
		return 0;
	}

	printf("Device %p name is %s\n", imu_dev, imu_dev->name);

	/* Setting scale in G, due to loss of precision if the SI unit m/s^2
	 * is used
	 */
	full_scale.val1 = 2;            /* G */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);


	/* Setting scale in degrees/s to match the sensor scale */
	full_scale.val1 = 500;          /* dps */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change sampling frequency to
	 * 0.0Hz before changing other attributes
	 */
	sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);
	return 0;
}