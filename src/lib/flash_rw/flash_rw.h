#pragma once

typedef struct net_config {
    uint8_t ipaddr[16];
    uint8_t netmask[16];
    uint8_t gateway[16];
} net_config_t;

typedef struct pid_config {
    float kp;
    float ki;
    float kd;
} pid_config_t;

typedef struct wifi_config {
    uint8_t ssid[16];
    uint8_t password[16];
} wifi_config_t;

typedef struct flash_rw_data {
    net_config_t net_config;
    pid_config_t pid_config;
    wifi_config_t wifi_config;
    uint8_t device_id;
}flash_rw_data_t;

int flash_rw_net_get(net_config_t *net_config);
int flash_rw_pid_get(pid_config_t *pid_config);
int flash_rw_device_id_get(uint8_t *device_id);
int flash_rw_wifi_get(wifi_config_t *wifi_config);
int flash_rw_ipaddr_set(uint8_t *ipaddr);
int flash_rw_netmask_set(uint8_t *netmask);
int flash_rw_gateway_set(uint8_t *gateway);
int flash_rw_pid_set(pid_config_t *pid_config);
int flash_rw_device_id_set(uint8_t device_id);
int flash_rw_wifi_set(wifi_config_t *wifi_config);