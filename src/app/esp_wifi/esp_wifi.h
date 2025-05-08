#pragma once

#define WIFI_STACK_SIZE 2048

void esp_wifi_init(void);
void esp_wifi_print_uart(uint8_t *buf, int len);
