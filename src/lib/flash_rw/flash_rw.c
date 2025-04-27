#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include "pressure_sensor.h"
#include "flash_rw.h"
#include "util.h"

LOG_MODULE_REGISTER(flash_rw, LOG_DEBUG);

static const struct device *flash_rw_dev = FIXED_PARTITION_DEVICE(rw_partition);
static off_t flash_rw_offset = FIXED_PARTITION_OFFSET(rw_partition);

int flash_rw_net_get(net_config_t *net_config)
{
    CHECK_NULL_ARG_AND_RETURN(net_config, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    *net_config = flash_rw_data.net_config;

    return 0;
}

int flash_rw_pid_get(pid_config_t *pid_config)
{
    CHECK_NULL_ARG_AND_RETURN(pid_config, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    *pid_config = flash_rw_data.pid_config;

    return 0;
}
int flash_rw_device_id_get(uint8_t *device_id)
{
    CHECK_NULL_ARG_AND_RETURN(device_id, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    *device_id = flash_rw_data.device_id;

    return 0;
}

int flash_rw_wifi_get(wifi_config_t *wifi_config)
{
    CHECK_NULL_ARG_AND_RETURN(wifi_config, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    *wifi_config = flash_rw_data.wifi_config;

    return 0;
}
int flash_rw_ipaddr_set(uint8_t *ipaddr)
{
    CHECK_NULL_ARG_AND_RETURN(ipaddr, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    memcpy(flash_rw_data.net_config.ipaddr, ipaddr, sizeof(flash_rw_data.net_config.ipaddr));
    
    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write ipaddr error: %d", ret);
        return ret;
    }
    return 0;
}
int flash_rw_netmask_set(uint8_t *netmask)
{
    CHECK_NULL_ARG_AND_RETURN(netmask, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    memcpy(flash_rw_data.net_config.netmask, netmask, sizeof(flash_rw_data.net_config.netmask));

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write netmask error: %d", ret);
        return ret;
    }
    return 0;
}
int flash_rw_gateway_set(uint8_t *gateway)
{
    CHECK_NULL_ARG_AND_RETURN(gateway, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    memcpy(flash_rw_data.net_config.gateway, gateway, sizeof(flash_rw_data.net_config.gateway));

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write gateway error: %d", ret);
        return ret;
    }
    return 0;
}
int flash_rw_pid_set(pid_config_t *pid_config)
{
    CHECK_NULL_ARG_AND_RETURN(pid_config, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    memcpy(&flash_rw_data.pid_config, pid_config, sizeof(flash_rw_data.pid_config));

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write pid error: %d", ret);
        return ret;
    }
    return 0;
}
int flash_rw_device_id_set(uint8_t device_id)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    flash_rw_data.device_id = device_id;

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write device id error: %d", ret);
        return ret;
    }
    return 0;
}

int flash_rw_wifi_set(wifi_config_t *wifi_config)
{
    CHECK_NULL_ARG_AND_RETURN(wifi_config, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    memcpy(&flash_rw_data.wifi_config, wifi_config, sizeof(flash_rw_data.wifi_config));

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write wifi error: %d", ret);
        return ret;
    }
    return 0;
}