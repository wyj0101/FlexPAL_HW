#include <stdio.h>

#include <zephyr/kernel.h>

#include "uart_handle.h"
#include "esp_wifi.h"
int main(void)
{
	uart_thread_init();
	esp_wifi_init();
	sensor_init();

	return 0;
}