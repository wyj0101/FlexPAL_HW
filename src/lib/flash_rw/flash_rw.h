#pragma once

#define NET_CONFIG_SIZE 16

// typedef struct net_config {
//     uint8_t ipaddr[NET_CONFIG_SIZE];
//     uint8_t netmask[NET_CONFIG_SIZE];
//     uint8_t gateway[NET_CONFIG_SIZE];
// } net_config_t;

typedef struct server_config {
    uint8_t ipaddr[NET_CONFIG_SIZE];
    uint32_t port;
} server_config_t;

typedef struct pid_config {
    float kp;
    float ki;
    float kd;
} pid_config_t;

typedef struct wifi_config {
    uint8_t ssid[NET_CONFIG_SIZE];
    uint8_t password[NET_CONFIG_SIZE];
} wifi_config_t;

typedef struct flash_rw_data {
    server_config_t server_config;
    pid_config_t pid_in_config;
    pid_config_t pid_out_config;
    wifi_config_t wifi_config;
    uint8_t device_id;
}flash_rw_data_t;

// int flash_rw_net_get(net_config_t *net_config);
int flash_rw_server_get(server_config_t *server_config);
int flash_rw_pid_get(pid_config_t *pid_in_config, pid_config_t *pid_out_config);
int flash_rw_device_id_get(uint8_t *device_id);
int flash_rw_wifi_get(wifi_config_t *wifi_config);

// int flash_rw_ipaddr_set(uint8_t *ipaddr);
// int flash_rw_netmask_set(uint8_t *netmask);
// int flash_rw_gateway_set(uint8_t *gateway);
int flash_rw_server_set(server_config_t *server_config);
int flash_rw_pid_in_set(pid_config_t pid_config);
int flash_rw_pid_out_set(pid_config_t pid_config);

int flash_rw_device_id_set(uint8_t device_id);
int flash_rw_wifi_ssid_set(uint8_t *ssid);
int falsh_rw_wifi_password_set(uint8_t *password);

int flash_rw_init(void);