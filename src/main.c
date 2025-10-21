#include <stdio.h>

#include <zephyr/kernel.h>

#include "uart_handle.h"
#include "esp_wifi.h"
#include "sensor.h"
#include "flash_rw.h"
#include "ldc161x.h"
#include "pid.h"
#include "imu.h"
#include "pmic.h"

int main(void)
{
	LDC161x_init();
	imu_init();
	pmic_init();
	flash_rw_init();
	pid_init();
	uart_thread_init();
	esp_wifi_init(); 
	return 0;
}