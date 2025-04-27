#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/sys/reboot.h>

#include "util.h"
#include "uart_handle.h"
#include "flash_rw.h"
#include "esp_wifi.h"

#define UART_NODE1 DT_ALIAS(uart1)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE1);

LOG_MODULE_REGISTER(uart, LOG_INFO);

#define MSG_SIZE 32
/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 5, 4);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

/*
 * Print a null-terminated string character by character to the UART interface
 */
// void print_uart(char *buf)
// {
// 	int msg_len = strlen(buf);

// 	for (int i = 0; i < msg_len; i++)
// 	{
// 		uart_poll_out(uart_dev, buf[i]);
// 	}
// }

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t rec_data;

	if (!uart_irq_update(uart_dev)) {
		LOG_ERR("UART IRQ update failed");
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		LOG_ERR("UART RX not ready");
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &rec_data, 1) == 1)
	{
		// if ((c == '\n' || c == '\r') && rx_buf_pos > 0)
		if ((rec_data == '\n') && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		}
		else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = rec_data;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

static void uart_handle(void *arug0, void *arug1, void *arug2)
{
	char data_buff[MSG_SIZE];
	char user_cmd[3];
	char user_sub_cmd[MSG_SIZE];
	char user_sub_action[MSG_SIZE];

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device is ready!");
		return;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

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
	uart_irq_rx_enable(uart_dev);

	LOG_DBG("start uart input");
	/* indefinitely wait for input from the user */
	while (k_msgq_get(&uart_msgq, &data_buff, K_FOREVER) == 0)
	{
		LOG_DBG("data_buff: %s\n", data_buff);
		sscanf(data_buff, "%s %s %s", user_cmd, user_sub_cmd, user_sub_action);
		LOG_DBG("user_cmd: %s, user_sub_cmd: %s, user_sub_action: %s", user_cmd, user_sub_cmd, user_sub_action);

		if (strcmp(user_cmd, "net") == 0) {
			if (strcmp(user_sub_cmd, "show") == 0) {
				net_config_t net_config;
				if (flash_rw_net_get(&net_config) == 0) {
					LOG_INF("\r\n net config \r\n gateway: %s \r\n ipaddr : %s \r\n netmask: %s",
							net_config.gateway, net_config.ipaddr, net_config.netmask);
				}
			} else if (strcmp(user_sub_cmd, "set") == 0) {
				if (strncmp(user_sub_action, "ipaddr=", strlen("ipaddr=")) == 0) {
					char *ipaddr = user_sub_action + strlen("ipaddr=");
					if (flash_rw_ipaddr_set(ipaddr) == 0) {
						LOG_INF("Set ipaddr success");
					}
				} else if (strncmp(user_sub_action, "netmask=", strlen("netmask=")) == 0) {
					char *netmask = user_sub_action + strlen("netmask=");
					if (flash_rw_netmask_set(netmask) == 0) {
						LOG_INF("Set netmask success");
					}
				} else if (strncmp(user_sub_action, "gateway=", strlen("gateway=")) == 0) {
					char *gateway = user_sub_action + strlen("gateway=");
					if (flash_rw_gateway_set(gateway) == 0) {
						LOG_INF("Set gateway success");
					}
				} else {
					LOG_ERR("Invalid net set command");
				}
			} else {
				LOG_ERR("Invalid net sub command");
			}
		} else if (strcmp(user_cmd, "pid") == 0) {
			pid_config_t pid_config;

			if (strcmp(user_sub_cmd, "get") == 0) {
				if (flash_rw_pid_get(&pid_config) == 0) {
					LOG_INF("\r\n pid config \r\n kp: %f \r\n ki: %f \r\n kd: %f",
							pid_config.kp, pid_config.ki, pid_config.kd);
				}
			} else if (strcmp(user_sub_cmd, "set") == 0) {
				if (sscanf(user_sub_action, "kp=%f", &pid_config.kp) == 1) {
					if (flash_rw_pid_set(&pid_config) == 0) {
						LOG_INF("Set kp success");
					}
				} else if (sscanf(user_sub_action, "ki=%f", &pid_config.ki) == 1) {
					if (flash_rw_pid_set(&pid_config) == 0) {
						LOG_INF("Set ki success");
					}
				} else if (sscanf(user_sub_action, "kd=%f", &pid_config.kd) == 1) {
					if (flash_rw_pid_set(&pid_config) == 0) {
						LOG_INF("Set kd success");
					}
				} else {
					LOG_ERR("Invalid pid set command");
				}
			} else {
				LOG_ERR("Invalid pid sub command");
			}
		} else if (strcmp(user_cmd, "sys") == 0) {
			if (strcmp(user_sub_cmd, "reboot") == 0) {
				LOG_INF("System reboot");
				sys_reboot(SYS_REBOOT_COLD);
			} else if (strcmp(user_sub_cmd, "set") == 0) {
				uint8_t device_id;
				if(sscanf(user_sub_action, "id=%hhd", &device_id) == 1) {
					if (flash_rw_device_id_set(device_id) == 0) {
						LOG_INF("Set device id success");
					}
				} else {
					LOG_ERR("Invalid sys command");
				}
			} else if (strcmp(user_sub_cmd, "get") == 0) { 
				uint8_t device_id;
				if (flash_rw_device_id_get(&device_id) == 0) {
					LOG_INF("Device id: %d", device_id);
				}
		 	} else {
				LOG_ERR("Invalid system command");
			}
		} else if (strcmp(user_cmd, "wifi") == 0) {
			if (strcmp(user_sub_cmd, "set") == 0) {
				wifi_config_t wifi_config;
				if (sscanf(user_sub_action, "ssid=%s", wifi_config.ssid) == 1) {
					if (flash_rw_wifi_set(&wifi_config) == 0) {
						LOG_INF("Set ssid success");
					}
				} else if (sscanf(user_sub_action, "pw=%s", wifi_config.password) == 1) {
					if (flash_rw_wifi_set(&wifi_config) == 0) {
						LOG_INF("Set password success");
					}
				} else {
					LOG_ERR("Invalid wifi set command");
				}
			} else if (strcmp(user_sub_cmd, "show") == 0) {
				wifi_config_t wifi_config;
				if (flash_rw_wifi_get(&wifi_config) == 0) {
					LOG_INF("\r\n wifi config \r\n ssid: %s \r\n pw: %s",
							wifi_config.ssid, wifi_config.password);
				}
			} else {
				LOG_ERR("Invalid wifi sub command");
			}
		} else if (strcmp(user_cmd, "esp") == 0) {
			esp_wifi_print_uart(user_sub_cmd);
			printf("cmd:%s\r\n", user_sub_cmd);
		} else if (strcmp(user_cmd, "help") == 0) {
			LOG_INF("Available commands: net, pid, sys");
		} else {
			LOG_ERR("Invalid command");
		}
	}
}

static struct k_thread uart_handle_thread;
static K_KERNEL_STACK_MEMBER(uart_handle_stack, SHELL_STACK_SIZE);

void uart_thread_init()
{
	k_thread_create(&uart_handle_thread, uart_handle_stack, K_THREAD_STACK_SIZEOF(uart_handle_stack),
					uart_handle, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0,
					K_NO_WAIT);
	return;
}