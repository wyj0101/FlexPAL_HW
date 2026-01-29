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

int flash_rw_server_get(server_config_t *server_config)
{
    CHECK_NULL_ARG_AND_RETURN(server_config, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    *server_config = flash_rw_data.server_config;

    return 0; 
}
/*
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
*/

int flash_rw_pid_get(pid_config_t *pid_in_config, pid_config_t *pid_out_config)
{
    CHECK_NULL_ARG_AND_RETURN(pid_in_config, EINVAL);
    CHECK_NULL_ARG_AND_RETURN(pid_out_config, EINVAL);

    flash_rw_data_t flash_rw_data;
    
    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    *pid_in_config = flash_rw_data.pid_in_config;
    *pid_out_config = flash_rw_data.pid_out_config;

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
/*
int flash_rw_ipaddr_set(uint8_t *ipaddr)
{
    CHECK_NULL_ARG_AND_RETURN(ipaddr, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (memcmp(flash_rw_data.net_config.ipaddr, ipaddr, sizeof(flash_rw_data.net_config.ipaddr)) == 0) {
        LOG_INF("ipaddr is same");
        return 0;
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
    
    if (memcmp(flash_rw_data.net_config.netmask, netmask, sizeof(flash_rw_data.net_config.netmask)) == 0) {
        LOG_INF("netmask is same");
        return 0;
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

    if (memcmp(flash_rw_data.net_config.gateway, gateway, sizeof(flash_rw_data.net_config.gateway)) == 0) {
        LOG_INF("gateway is same");
        return 0;
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
*/
int flash_rw_server_set(server_config_t *server_config)
{
    CHECK_NULL_ARG_AND_RETURN(server_config, EINVAL);

    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (memcmp(&flash_rw_data.server_config, server_config, sizeof(flash_rw_data.server_config)) == 0) {
        LOG_INF("server config is same");
        return 0;
    }

    memcpy(&flash_rw_data.server_config, server_config, sizeof(flash_rw_data.server_config));

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write server config error: %d", ret);
        return ret;
    }
    return 0;
}
#if 0
int flash_rw_pid_kp_set(float pid_kp)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (flash_rw_data.pid_config.kp == pid_kp) {
        LOG_INF("pid kp is same");
        return 0;
    }

    flash_rw_data.pid_config.kp = pid_kp;

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write pid kp error: %d", ret);
        return ret;
    }
    return 0;
}

int flash_rw_pid_ki_set(float pid_ki)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (flash_rw_data.pid_config.ki == pid_ki) {
        LOG_INF("pid ki is same");
        return 0;
    }

    flash_rw_data.pid_config.ki = pid_ki;

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write pid ki error: %d", ret);
        return ret;
    }
    return 0;
}

int flash_rw_pid_kd_set(float pid_kd)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (flash_rw_data.pid_config.kd == pid_kd) {
        LOG_INF("pid kd is same");
        return 0;
    }

    flash_rw_data.pid_config.kd = pid_kd;

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write pid kd error: %d", ret);
        return ret;
    }
    return 0;
}
#endif
int flash_rw_pid_in_set(pid_config_t pid_config)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (memcmp(&flash_rw_data.pid_in_config, &pid_config, sizeof(flash_rw_data.pid_in_config)) == 0) {
        LOG_INF("pid in config is same");
        return 0;
    }

    memcpy(&flash_rw_data.pid_in_config, &pid_config, sizeof(flash_rw_data.pid_in_config));

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write pid in config error: %d", ret);
        return ret;
    }
    return 0;
}
int flash_rw_pid_out_set(pid_config_t pid_config)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (memcmp(&flash_rw_data.pid_out_config, &pid_config, sizeof(flash_rw_data.pid_out_config)) == 0) {
        LOG_INF("pid out config is same");
        return 0;
    }

    memcpy(&flash_rw_data.pid_out_config, &pid_config, sizeof(flash_rw_data.pid_out_config));

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write pid out config error: %d", ret);
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
    
    if (flash_rw_data.device_id == device_id) {
        LOG_INF("device id is same");
        return 0;
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

int flash_rw_wifi_ssid_set(uint8_t *ssid)
{
    CHECK_NULL_ARG_AND_RETURN(ssid, EINVAL);
    
    flash_rw_data_t flash_rw_data; 

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (memcmp(flash_rw_data.wifi_config.ssid, ssid, sizeof(flash_rw_data.wifi_config.ssid)) == 0) {
        LOG_INF("ssid is same");
        return 0;
    }
    memcpy(flash_rw_data.wifi_config.ssid, ssid, sizeof(flash_rw_data.wifi_config.ssid));

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

int falsh_rw_wifi_password_set(uint8_t *password)
{
    CHECK_NULL_ARG_AND_RETURN(password, EINVAL);
    
    flash_rw_data_t flash_rw_data; 

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

    if (memcmp(flash_rw_data.wifi_config.password, password, sizeof(flash_rw_data.wifi_config.password)) == 0) {
        LOG_INF("password is same");
        return 0;
    }
    memcpy(flash_rw_data.wifi_config.password, password, sizeof(flash_rw_data.wifi_config.password));

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

int flash_rw_ldc_max_value_set(uint32_t ldc_max_value)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    
    if (flash_rw_data.ldc_max_value == ldc_max_value) {
        LOG_INF("ldc max value is same");
        return 0;
    }

    flash_rw_data.ldc_max_value = ldc_max_value;

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write ldc max value error: %d", ret);
        return ret;
    }
    return 0;
}
int flash_rw_ldc_max_value_get(uint32_t *ldc_max_value)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    *ldc_max_value = flash_rw_data.ldc_max_value;
    return 0;
}

int flash_rw_pressure_offset_value_set(float pressure_offset_value)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    
    if (flash_rw_data.pressure_offset_value == pressure_offset_value) {
        LOG_INF("pressure offset value is same");
        return 0;
    }

    flash_rw_data.pressure_offset_value = pressure_offset_value;

    ret = flash_erase(flash_rw_dev, flash_rw_offset, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash erase error: %d", ret);
        return ret;
    }

    ret = flash_write(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash write pressure offset value error: %d", ret);
        return ret;
    }
    return 0;
}
int flash_rw_pressure_offset_value_get(float *pressure_offset_value)
{
    flash_rw_data_t flash_rw_data;

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_data, sizeof(flash_rw_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }
    *pressure_offset_value = flash_rw_data.pressure_offset_value;
    return 0;
}
int flash_rw_init(void)
{
    flash_rw_data_t flash_rw_init_data;
    uint8_t flash_init_data[NET_CONFIG_SIZE];
    memset(flash_init_data, 0xFF, sizeof(flash_init_data));

    if (!device_is_ready(flash_rw_dev)) {
        LOG_ERR("flash device is not ready");
        return -1;
    }

    int ret = flash_read(flash_rw_dev, flash_rw_offset, &flash_rw_init_data, sizeof(flash_rw_init_data));
    if (ret < 0) {
        LOG_ERR("flash read error: %d", ret);
        return ret;
    }

/*
    if (memcmp(flash_rw_init_data.net_config.ipaddr, flash_init_data, sizeof(flash_rw_init_data.net_config.ipaddr)) == 0) {
       flash_rw_ipaddr_set("192.168.137,10");
    }
    if (memcmp(flash_rw_init_data.net_config.netmask, flash_init_data, sizeof(flash_rw_init_data.net_config.netmask)) == 0) {
       flash_rw_netmask_set("255.255.255.0");
    }
    if (memcmp(flash_rw_init_data.net_config.gateway, flash_init_data, sizeof(flash_rw_init_data.net_config.gateway)) == 0) {
        flash_rw_gateway_set("192.168.137.1");
    }
*/
    if (memcmp(flash_rw_init_data.server_config.ipaddr, flash_init_data, sizeof(flash_rw_init_data.server_config.ipaddr)) == 0) {
        server_config_t server_config = {0};
        memcpy(server_config.ipaddr, "192.168.137.255", sizeof(server_config.ipaddr));
        server_config.port = 5005;
        flash_rw_server_set(&server_config);
    }

    if (memcmp(flash_rw_init_data.wifi_config.ssid, flash_init_data, sizeof(flash_rw_init_data.wifi_config.ssid)) == 0) {
        flash_rw_wifi_ssid_set("FlexPAL_Hotspot");
    }
    if (memcmp(flash_rw_init_data.wifi_config.password, flash_init_data, sizeof(flash_rw_init_data.wifi_config.password)) == 0) {
        falsh_rw_wifi_password_set("12345678");
    }

    pid_config_t pid_init_config = {.kd = 1.0, .ki = 0.0, .kp = 0.0};
    if (memcmp(&flash_rw_init_data.pid_in_config, flash_init_data, sizeof(flash_rw_init_data.pid_in_config)) == 0) {
        flash_rw_pid_in_set(pid_init_config);
    }
    if (memcmp(&flash_rw_init_data.pid_out_config, flash_init_data, sizeof(flash_rw_init_data.pid_out_config)) == 0) {
        flash_rw_pid_out_set(pid_init_config);
    }

    if (memcmp(&flash_rw_init_data.device_id, flash_init_data, sizeof(flash_rw_init_data.device_id)) == 0) {
        flash_rw_device_id_set(1);
    }

    if (memcmp(&flash_rw_init_data.ldc_max_value, flash_init_data, sizeof(flash_rw_init_data.ldc_max_value)) == 0) {
        flash_rw_init_data.ldc_max_value = 182260000;
        flash_rw_ldc_max_value_set(flash_rw_init_data.ldc_max_value);
    }
    if (memcmp(&flash_rw_init_data.pressure_offset_value, flash_init_data, sizeof(flash_rw_init_data.pressure_offset_value)) == 0) {
        flash_rw_init_data.pressure_offset_value = 0.0;
        flash_rw_pressure_offset_value_set(flash_rw_init_data.pressure_offset_value);
    }

    return 0;
}