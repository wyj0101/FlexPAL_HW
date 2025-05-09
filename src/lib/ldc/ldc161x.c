#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#if CONFIG_LOG
#include <zephyr/logging/log.h>
#endif
#include "logging.h"

#include "ldc161x.h"

const char *status_str[]={"conversion under range error","conversion over range error",
    "watch dog timeout error","Amplitude High Error",
    "Amplitude Low Error","Zero Count Error",
    "Data Ready","unread conversion is present for channel 0",
    " unread conversion is present for Channel 1.",
    "unread conversion ispresent for Channel 2.",
    "unread conversion is present for Channel 3."};

static const struct i2c_dt_spec ldc_dev = I2C_DT_SPEC_GET(DT_NODELABEL(ldc));

LOG_MODULE_REGISTER(ldc161x, LOG_DEBUG);

int LDC161x_write(uint8_t reg_addr, uint16_t data)
{
    uint8_t buf[2];
    buf[0] = (data >> 8) & 0xFF;
    buf[1] = data & 0xFF;

    return i2c_burst_write_dt(&ldc_dev, reg_addr, buf, sizeof(buf));
}
int LDC161x_read(uint8_t reg_addr, uint16_t *data)
{
    uint8_t buf[2];
    int ret = i2c_burst_read_dt(&ldc_dev, reg_addr, buf, sizeof(buf));
    if (ret < 0) {
        LOG_ERR("I2C read error: %d", ret);
        return ret;
    }
    *data = (buf[0] << 8) | buf[1];
    return 0;
}

int LDC161x_read_4bytes(uint8_t reg_addr, uint32_t *data)
{
    uint8_t buf[4];

    while (!i2c_is_ready_dt(&ldc_dev)) {
        LOG_ERR("I2C device is not ready\n");
    }

    int ret = i2c_burst_read_dt(&ldc_dev, reg_addr, buf, sizeof(buf));
    if (ret < 0) {
        LOG_ERR("I2C read 4 bytes error: %d", ret);
        return ret;
    }
    *data = ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
    return 0;
}

int LDC161x_conversion_time_set(uint8_t channel, uint16_t time)
{
    if (channel > 3) {
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    uint8_t reg_addr = LDC_REG_SET_CONVERSION_TIME_REG_START + channel;

    return LDC161x_write(reg_addr, time);
}
int LDC161x_stabilize_time_set(uint8_t channel, uint16_t time)
{ 
    if (channel > 3) {
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    uint8_t reg_addr = LDC_REG_SET_LC_STABILIZE_REG_START + channel;

    return LDC161x_write(reg_addr, time);
}

int LDC161x_Fin_clock_set(uint8_t channel, uint16_t divider)
{
    if (channel > 3) {
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    uint8_t reg_addr = LDC_REG_SET_FREQ_REG_START + channel;

    return LDC161x_write(reg_addr, divider);
}

int LDC161x_mux_config_set(uint8_t channel, uint16_t config)
{
    if (channel > 3) {
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    uint8_t reg_addr = LDC_REG_MUL_CONFIG_REG + channel;

    return LDC161x_write(reg_addr, config);
}

int LDC161x_driver_current_set(uint8_t channel, uint16_t current)
{
    if (channel > 3) {
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    uint8_t reg_addr = LDC_REG_SET_DRIVER_CURRENT_REG + channel;

    return LDC161x_write(reg_addr, current);
}

int LDC161x_error_config_set(uint8_t channel, uint16_t config)
{
    if (channel > 3) {
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    uint8_t reg_addr = LDC_REG_ERROR_CONFIG_REG + channel;

    return LDC161x_write(reg_addr, config);
}

int LDC161x_sensor_config_set(uint8_t channel, uint16_t config)
{
    if (channel > 3) {
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    uint8_t reg_addr = LDC_REG_SENSOR_CONFIG_REG + channel;

    return LDC161x_write(reg_addr, config);
}

int LDC161x_read_value(uint8_t channel, uint32_t *value)
{
    if (channel > 3) {
        LOG_ERR("Invalid channel: %d", channel);
        goto END;
    }

    uint8_t reg_addr = LDC_REG_CONVERTION_RESULT_REG_START + (channel * 2);
    uint32_t data;
    int ret = LDC161x_read_4bytes(reg_addr, &data);
    if (ret < 0) {
        LOG_ERR("I2C read error: %d", ret);
        goto END;
    }

    if (data == 0xffffffff) {
        LOG_ERR("can't detect coil Coil Inductance!!!");
        goto END;
    }

    uint8_t status = (data >> 24) & 0xF0;
    if (status != 0) {
        if (status & 0x80) {
            LOG_ERR("ERR_UR-Under range error!");
        }
        if (status & 0x40) {
            LOG_ERR("ERR_OR-Over range error!");
        }
        if (status & 0x20) {
            LOG_ERR("ERR_WDT-Watch dog timeout error!");
        }
        if (status & 0x10) {
            LOG_ERR("ERR_AE Error!");
        }
        goto END;
    }
    *value = data & 0x0FFFFFFF;
    return 0;

END:
    *value = 0;
    return -EINVAL;
}
int LDC161x_init(void)
{
    uint8_t i;
    uint16_t data;

    if (!i2c_is_ready_dt(&ldc_dev)) {
        LOG_ERR("I2C device is not ready\n");
        return -ENODEV;
    }

    LDC161x_read(LDC_REG_READ_DEVICE_ID, &data);
    if (data != LDC161X_DEVICE_ID) {
        LOG_ERR("LDC161x device ID mismatch: expected 0x%04X, got 0x%04X\n", LDC161X_DEVICE_ID, data);
        // return -EINVAL;
    }

    for (i = 0; i < CONFIG_LDC_CHANNEL_NUM; i++) {
        LDC161x_conversion_time_set(i, LDC_RECOUNT_VALUE(0x0546));
        // LDC161x_conversion_time_set(i, 0x0546);
    }

    for (i = 0; i < CONFIG_LDC_CHANNEL_NUM; i++) {
        LDC161x_stabilize_time_set(i, LDC_SETTLECOUNT_VALUE(100));
        // LDC161x_stabilize_time_set(i, 100);
    }

    for (i = 0; i < CONFIG_LDC_CHANNEL_NUM; i++) {
        LDC161x_Fin_clock_set(i, LDC_CLOCK_DIVIDERS_FIN(0x1) | LDC_CLOCK_DIVIDERS_FREF(0x1));
        // LDC161x_Fin_clock_set(i, 0x1001);
    }

    for (i = 0; i < CONFIG_LDC_CHANNEL_NUM; i++) {
        LDC161x_mux_config_set(i, LDC_MUX_CONFIG_AUTOSCAN_EN | LDC_MUX_CONFIG_RR_SEQUENCE(0x2) | LDC_MUX_CONFIG_DEGLITCH(0x1));
        // LDC161x_mux_config_set(i, 0x8209);
    }

    for (i = 0; i < CONFIG_LDC_CHANNEL_NUM; i++) {
        // LDC161x_driver_current_set(i, LDC_DRIVE_CURRENT_IDRIVE(0xf));
        LDC161x_driver_current_set(i, 0x7800);
    }

    // for (i = 0; i < CONFIG_LDC_CHANNEL_NUM; i++) {
    //     LDC161x_error_config_set(i, LDC_ERROR_CONFIG_ENABLE(0x1));
    // }

    for (i = 0; i < CONFIG_LDC_CHANNEL_NUM; i++) {
        LDC161x_sensor_config_set(i, 0x1401);
    }

    return 0;
}
