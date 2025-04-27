#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include "logging.h"
#if CONFIG_LOG
#include <zephyr/logging/log.h>
#endif

#include "imu.h"
#include "pressure_sensor.h"
#include "pump_ctrl.h"
#include "sensor.h"

LOG_MODULE_REGISTER(sensor, LOG_DEBUG);

static struct k_thread sensor_handle_thread;
static K_KERNEL_STACK_MEMBER(sensor_handle_stack, SENSOR_STACK_SIZE);

extern float pressure_sensor_value;

static void sensor_handle(void *arug0, void *arug1, void *arug2)
{
    imu_data imu_value;
    pressure_sensor_init();

    while (1)
    {
        get_imu_value(&imu_value);
        printf("pressure: %f x:%.2f y:%.2f z:%.2f gx:%.2f gy:%.2f gx:%.2f temp:%.2f\n", pressure_sensor_value,
                imu_value.acce_x, imu_value.acce_y, imu_value.acce_z,
               imu_value.gyro_x, imu_value.gyro_y, imu_value.gyro_z,
               imu_value.temp);
        k_sleep(K_MSEC(300));
    }
}
void sensor_init()
{
    k_thread_create(&sensor_handle_thread, sensor_handle_stack, K_THREAD_STACK_SIZEOF(sensor_handle_stack),
                    sensor_handle, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0,
                    K_NO_WAIT);
}