#pragma once

#define WIFI_STACK_SIZE 1024

void esp_wifi_init(void);
void esp_wifi_print_uart(char *buf);
