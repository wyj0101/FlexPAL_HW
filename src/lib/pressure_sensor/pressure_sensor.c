#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>

#include "logging.h"
#if CONFIG_LOG
#include <zephyr/logging/log.h>
#endif

#include "pressure_sensor.h"
#include "flash_rw.h"
#include "pid.h"
#include "pump_ctrl.h"

LOG_MODULE_REGISTER(pressure_sensor, LOG_DEBUG);

static struct k_thread pressure_sensor_handle_thread;
static K_KERNEL_STACK_MEMBER(pressure_sensor_handle_stack, PRESSURE_SENSOR_STACK_SIZE);

static const struct device *const spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi2));
static struct gpio_dt_spec sensor_cs =  SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(pressure_sensor));

float pressure_sensor_value = 0;
float pressure_sensor_offset_value = 0;
bool spring_pid_enable = false;
float spring_pid_pressure_value = 0;
bool pressure_sensor_enable = true;

static struct spi_config sensor_cfg = {0};

static uint8_t start_cmd[] = {0xaa, 0x00, 0x00};
static uint8_t read_cmd[] = {0x00, 0x00, 0x00, 0x00};

static uint8_t value_buff[4] = {0};

static struct spi_buf start_cmd_buff = {
    .buf = start_cmd,
    .len = sizeof(start_cmd),
};

static struct spi_buf read_cmd_buff = {
    .buf = read_cmd,
    .len = sizeof(read_cmd),
};

static struct spi_buf read_value_buff = {
    .buf = value_buff,
    .len = sizeof(value_buff),
};

const static struct spi_buf_set start_cmd_set = {
    .buffers = &start_cmd_buff,
    .count = 1,
};

const static struct spi_buf_set read_cmd_set = {
    .buffers = &read_cmd_buff,
    .count = 1,
};

const static struct spi_buf_set read_value_set = {
    .buffers = &read_value_buff,
    .count = 1,
};

static int sensor_config_init(void)
{
    sensor_cfg.frequency = SPI_FREQUENCY;
    sensor_cfg.operation = SPI_OPERTION;
    sensor_cfg.cs.gpio = sensor_cs;

    return 0;
}

static void pressure_sensor_handle(void *arug0, void *arug1, void *arug2)
{
    float value;
    int count = 0;
    float pid_output = 0;
    int retry_count = 0, retry_write_count = 0;

    LOG_INF("sensor handle start!");
    sensor_config_init();

    if (!device_is_ready(spi_dev))
    {
        LOG_ERR("spi device not ready");
        return;
    }
    flash_rw_pressure_offset_value_get(&pressure_sensor_offset_value);

    if (spi_transceive(spi_dev, &sensor_cfg, &read_cmd_set, &read_value_set) != 0) {
            LOG_ERR("Spi Read Failed!");
    }
    k_msleep(10);

    while (1)
    {
    if (pressure_sensor_enable) {

retry_write:
        if (spi_transceive(spi_dev, &sensor_cfg, &start_cmd_set, &read_value_set) != 0) {
            LOG_ERR("Spi Write Failed!");
        }

        if (value_buff[0] != 0x40) {
            k_msleep(1);
            retry_write_count++;
            if (retry_write_count >= 11) {
                LOG_ERR("Pressure sensor start command failed too many times!");
                retry_write_count = 0;
                continue;
                pressure_sensor_value = 0;
            }
            goto retry_write;
        }

        // 经测试，最少延时7ms，6900us都不行
        k_msleep(7);

retry:
        if (spi_transceive(spi_dev, &sensor_cfg, &read_cmd_set, &read_value_set) != 0) {
            LOG_ERR("Spi Read Failed!");
        }

        // 经测试，读完之后，必须加点延时才能进行写操作
        k_msleep(1);
        if (value_buff[0] == 0x60) {
            retry_count++;
            if (retry_count >= 5) {
                LOG_ERR("Pressure sensor read failed too many times!");
                retry_count = 0;
                continue;
                pressure_sensor_value = 0;
            }
            goto retry;
        }
        value = (value_buff[1] << 16) | (value_buff[2] << 8) | value_buff[3];
        pressure_sensor_value = ((((value - 0x800000) * 0xc8) / 0xb33333) * 1000) - pressure_sensor_offset_value;

        if (spring_pid_enable) {
            count++;
            pid_output = pid_calculate_output(pressure_sensor_value, spring_pid_pressure_value);
            pump_ctrl_set(pid_output);

            if (count >= 5) {
                count = 0;
                spring_pid_enable = false;
            }
        }
    }
    else {
        k_msleep(1000);
    }
}
void pressure_sensor_init()
{
    gpio_pin_configure_dt(&sensor_cs, GPIO_OUTPUT_LOW);
    k_thread_create(&pressure_sensor_handle_thread, pressure_sensor_handle_stack, K_THREAD_STACK_SIZEOF(pressure_sensor_handle_stack),
                    pressure_sensor_handle, NULL, NULL, NULL, 10, 0,
                    K_NO_WAIT);
}