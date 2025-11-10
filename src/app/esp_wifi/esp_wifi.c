#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/sys/reboot.h>

#include "util.h"
#include "esp_wifi.h"
#include "flash_rw.h"
#include "pump_ctrl.h"
#include "sensor.h"
#include "pid.h"

static const struct device *const esp_dev = DEVICE_DT_GET(DT_ALIAS(uart2));

LOG_MODULE_REGISTER(wifi, LOG_DEBUG);

#define WIFI_MSG_SIZE 40
/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(wifi_msgq, WIFI_MSG_SIZE, 5, 4);

/* receive buffer used in UART ISR callback */
static uint8_t rx_buf[WIFI_MSG_SIZE];
static int rx_buf_pos;
extern bool spring_pid_enable;

/*
 * Print a null-terminated string character by character to the UART interface
 */
void esp_wifi_print_uart(uint8_t *buf, int len)
{
	for (int i = 0; i < len; i++)
	{
		uart_poll_out(esp_dev, buf[i]);
	}
}

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void wifi_serial_cb(const struct device *dev, void *user_data)
{
	uint8_t rec_data;

	if (!uart_irq_update(esp_dev)) {
		LOG_ERR("UART IRQ update failed");
		return;
	}

	// if (!uart_irq_rx_ready(esp_dev)) {
	// 	LOG_ERR("UART RX not ready");
	// 	return;
	// }

	/* read until FIFO empty */
	while (uart_fifo_read(esp_dev, &rec_data, 1) == 1)
	{
		// if ((rec_data == '\n' || rec_data == '\r') && rx_buf_pos > 0) {
		if ((rec_data == '\n') && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos - 1] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&wifi_msgq, &rx_buf, K_NO_WAIT);
			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
			memset(rx_buf, 0, sizeof(rx_buf));
		}
		else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = rec_data;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

/*
 * Send AT command to ESP8266 and wait for response
 * @param cmd: AT command to send
 * @param len: length of the command
 * @param timeout: timeout in milliseconds,
 * @return 0 on success, -1 on failure
 */
static int esp_at_send_cmd(char *cmd, int len, int timeout)
{
	uint8_t rec_data_buff[WIFI_MSG_SIZE];
	uint8_t i;
	
	esp_wifi_print_uart((uint8_t *)cmd, len);
	// 经测试，at命令最多返回七个数据包
	for(i = 0; i < 7; i++) {
		k_msgq_get(&wifi_msgq, &rec_data_buff, K_MSEC(timeout));
		if (strcmp(rec_data_buff, "OK") == 0) {
			LOG_INF("esp_at_send_cmd success: %s", rec_data_buff);
			return 0;
		}
		if (strcmp(rec_data_buff, "ERROR") == 0) {
			esp_wifi_print_uart((uint8_t *)cmd, len);
		}
	}
	LOG_ERR("at failed cmd:%s rec:%s", cmd, rec_data_buff);
	return -1;
}
static int esp_at_wifi_init(void)
{
	int ret = -1;
	uint8_t at_buff[64] = {0};
	wifi_config_t wifi_config;
	server_config_t server_config;

	flash_rw_wifi_get(&wifi_config);
	flash_rw_server_get(&server_config);

	ret = esp_at_send_cmd("+++", (sizeof("+++") - 1), 1000);	// disconnect udp send
	ret = esp_at_send_cmd("AT\r\n", sizeof("AT\r\n"), 1000);	// test at
	ret = esp_at_send_cmd("AT+CWMODE=1\r\n", sizeof("AT+CWMODE=1\r\n"), 1000); // set wifi mode station
	snprintf(at_buff, sizeof(at_buff), "AT+CWJAP=\"%s\",\"%s\"\r\n", wifi_config.ssid, wifi_config.password);
	ret = esp_at_send_cmd(at_buff, strlen(at_buff), 3000); // connect wifi
	memset(at_buff, 0, sizeof(at_buff));
	snprintf(at_buff, sizeof(at_buff), "AT+CIPSTART=\"UDP\",\"%s\",%d,%d,0\r\n", server_config.ipaddr, server_config.port, server_config.port);
	ret = esp_at_send_cmd(at_buff, strlen(at_buff), 3000); // connect udp
	ret = esp_at_send_cmd("AT+CIPMODE=1\r\n", sizeof("AT+CIPMODE=1\r\n"), 1000); // 使能透传
	ret = esp_at_send_cmd("AT+CIPSEND\r\n", sizeof("AT+CIPSEND\r\n"), 1000); // 进入透传模式
	
	return 0;
}

extern float pressure_sensor_value;
extern bool sensor_debug_flag;
extern bool pressure_pid_enable;
extern float ldc_length;
extern float spring_pid_pressure_value;

static void wifi_handle(void *arug0, void *arug1, void *arug2)
{
	uint8_t data_buff[WIFI_MSG_SIZE];

	if (!device_is_ready(esp_dev)) {
		LOG_ERR("UART device is ready!");
		return;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(esp_dev, wifi_serial_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			LOG_ERR("Interrupt-driven UART API support not enabled");
		} else if (ret == -ENOSYS) {
			LOG_ERR("UART device does not support interrupt-driven API");
		} else {
			LOG_ERR("Error setting UART callback: %d", ret);
		}
		return;
	}
	uart_irq_rx_enable(esp_dev);

	uint8_t device_id = 0;
	int32_t target_value = 0;
	float pid_output = 0;

    flash_rw_device_id_get(&device_id);

	pump_ctrl_init();

	k_msleep(2000);
	esp_at_wifi_init();

	sensor_init();
	/* indefinitely wait for input from the user */
	while (k_msgq_get(&wifi_msgq, &data_buff, K_FOREVER) == 0)
	{
		if ((data_buff[0] != 1) && (data_buff[0] != 2)) {
			LOG_ERR("Invalid UDP data");
			continue;
		}

		if (data_buff[0] == 1) {
			memcpy(&target_value, &data_buff[(1 + (device_id - 1)* 4)], sizeof(target_value));
			pid_output = pid_calculate_output(pressure_sensor_value, target_value);
			pump_ctrl_set(pid_output);
		} else if (data_buff[0] == 2) {
			target_value = data_buff[(1 + (device_id - 1)* 4)];
			pump_ctrl_set(target_value);
		} else if (data_buff[0] == 3) {
			memcpy(&target_value, &data_buff[(1 + (device_id - 1)* 4)], sizeof(target_value));
			if (!spring_pid_enable) {
				spring_pid_pressure_value = spring_pid_calculate_output(ldc_length, target_value);
				spring_pid_enable = true;
			}	
		}

		if (sensor_debug_flag) {
			printf("device_id: %d, target_value: %d pid_out:%.2f\n",
				device_id, target_value, pid_output);
		}
	}
}

static struct k_thread wifi_handle_thread;
static K_KERNEL_STACK_MEMBER(wifi_handle_stack, WIFI_STACK_SIZE);

void esp_wifi_init()
{
	k_thread_create(&wifi_handle_thread, wifi_handle_stack, K_THREAD_STACK_SIZEOF(wifi_handle_stack),
					wifi_handle, NULL, NULL, NULL, 16, 0,
					K_NO_WAIT);
	return;
}