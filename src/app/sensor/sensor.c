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


static void sensor_handle(void *arug0, void *arug1, void *arug2)
{


    while (1)
    {
        
    }
}
void sensor_init()
{
    k_thread_create(&sensor_handle_thread, sensor_handle_stack, K_THREAD_STACK_SIZEOF(sensor_handle_stack),
                    sensor_handle, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0,
                    K_NO_WAIT);
}