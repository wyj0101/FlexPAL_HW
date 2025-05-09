#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#if CONFIG_LOG
#include <zephyr/logging/log.h>
#endif

#include "logging.h"
#include "imu.h"
#include "pressure_sensor.h"
#include "pump_ctrl.h"
#include "sensor.h"
#include "sys_adc.h"
#include "esp_wifi.h"
#include "flash_rw.h"
#include "ldc161x.h"


static struct k_thread sensor_handle_thread;
static K_KERNEL_STACK_MEMBER(sensor_handle_stack, SENSOR_STACK_SIZE);

extern bool sensor_debug_flag;
extern float pressure_sensor_value;
static uint8_t udp_send_buff[40];
static void sensor_handle(void *arug0, void *arug1, void *arug2)
{
    imu_data imu_value = {0};
    uint8_t device_id = 1;
    float battery_value = 0;
    uint32_t ldc_length = 0;

    flash_rw_device_id_get(&device_id);

    pressure_sensor_init();
    sys_adc_init();

    while (1)
    {
        get_imu_value(&imu_value);
        battery_value = ((sys_adc_read(ADC_CHANNEL_BAT) / 1000.0f) - 2.8) / 1.40f * 100.0f;

        LDC161x_read_value(0, &ldc_length);

        udp_send_buff[0] = device_id;
        memcpy(&udp_send_buff[1], &ldc_length, sizeof(ldc_length));
        memcpy(&udp_send_buff[5], &imu_value, sizeof(imu_value));
        memcpy(&udp_send_buff[29], &pressure_sensor_value, sizeof(pressure_sensor_value));
        memcpy(&udp_send_buff[33], &battery_value, sizeof(battery_value));
        
        esp_wifi_print_uart(udp_send_buff, sizeof(udp_send_buff));
        
        if (sensor_debug_flag) {
            printf("ldc:%u pressure: %f x:%.2f y:%.2f z:%.2f gx:%.2f gy:%.2f gx:%.2f temp:%.2f bat:%.2f bus:%d\n",
                    ldc_length, pressure_sensor_value, imu_value.acce_x.value, imu_value.acce_y.value, imu_value.acce_z.value,
                    imu_value.gyro_x.value, imu_value.gyro_y.value, imu_value.gyro_z.value, imu_value.temp.value, 
                    battery_value, sys_adc_read(ADC_CHANNEL_BUS) * 2);
           
        }

        k_sleep(K_MSEC(10));
    }
}
void sensor_init()
{
    k_thread_create(&sensor_handle_thread, sensor_handle_stack, K_THREAD_STACK_SIZEOF(sensor_handle_stack),
                    sensor_handle, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0,
                    K_NO_WAIT);
}