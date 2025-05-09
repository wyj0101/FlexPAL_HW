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
#include "pump_ctrl.h"

#define UART_NODE1 DT_ALIAS(uart1)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE1);

LOG_MODULE_REGISTER(uart, LOG_DEBUG);

#define MSG_SIZE 64
/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 5, 4);

/* receive buffer used in UART ISR callback */
static uint8_t rx_buf[MSG_SIZE];
static int rx_buf_pos;

bool sensor_debug_flag = false;
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
	uint8_t data_buff[MSG_SIZE];
	uint8_t user_cmd[3];
	uint8_t user_sub_cmd[MSG_SIZE];
	uint8_t user_sub_action[MSG_SIZE];

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
/*
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
*/
		} else if (strcmp(user_cmd, "server") == 0) {
			if (strcmp(user_sub_cmd, "get") == 0) {
				server_config_t server_config;
				if (flash_rw_server_get(&server_config) == 0) {
					LOG_INF("\r\n server config \r\n ipaddr: %s \r\n port: %d",
							server_config.ipaddr, server_config.port);
				}
			} else if (strcmp(user_sub_cmd, "set") == 0) {
				server_config_t server_config = {0};
				if (sscanf(user_sub_action, "port=%u,ipaddr=%s", &server_config.port, server_config.ipaddr) == 2) {
					if (flash_rw_server_set(&server_config) == 0) {
						LOG_INF("Set server config success");
					}
				} else {
					LOG_ERR("Invalid server set command");
				}
			} else {
				LOG_ERR("Invalid server sub command");
			}
		} else if (strcmp(user_cmd, "pid") == 0) {
			pid_config_t pid_in, pid_out;

			if (strcmp(user_sub_cmd, "get") == 0) {
				if (flash_rw_pid_get(&pid_in, &pid_out) == 0) {
					LOG_INF("\r\n pid_in config \r\n kp: %f \r\n ki: %f \r\n kd: %f \r\n pid_out config \r\n kp: %f \r\n ki: %f \r\n kd: %f",
							pid_in.kp, pid_in.ki, pid_in.kd, pid_out.kp, pid_out.ki, pid_out.kd);
				}
			} else if (strcmp(user_sub_cmd, "set_in") == 0) {
				if (sscanf(user_sub_action, "kp=%f,ki=%f,kd=%f", &pid_in.kp, &pid_in.ki, &pid_in.kd) == 3) {
					flash_rw_pid_in_set(pid_in);
					LOG_INF("Set pid_in config success");
				} else {
					LOG_ERR("Invalid pid set input command");
				}
			} else if (strcmp(user_sub_cmd, "set_out") == 0) {
				if (sscanf(user_sub_action, "kp=%f,ki=%f,kd=%f", &pid_out.kp, &pid_out.ki, &pid_out.kd) == 3) {
					flash_rw_pid_out_set(pid_out);
					LOG_INF("Set pid_out config success");
				} else {
					LOG_ERR("Invalid pid set output command");
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
					if (flash_rw_wifi_ssid_set(wifi_config.ssid) == 0) {
						LOG_INF("Set ssid success");
					}
				} else if (sscanf(user_sub_action, "pw=%s", wifi_config.password) == 1) {
					if (falsh_rw_wifi_password_set(wifi_config.password) == 0) {
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
			size_t len = strlen((char *)user_sub_cmd); // 获取当前字符串长度
			if (len + 2 < sizeof(user_sub_cmd)) {      // 确保不会超出数组大小
				user_sub_cmd[len] = '\r';              // 添加 '\r'
				user_sub_cmd[len + 1] = '\n';          // 添加 '\n'
				user_sub_cmd[len + 2] = '\0';          // 添加字符串终止符 '\0'
			}
			esp_wifi_print_uart(user_sub_cmd, strlen(user_sub_cmd));
		} else if (strcmp(user_cmd, "pump") == 0) {
			float value;
			if (sscanf(user_sub_cmd, "set=%f", &value) == 1) {
				if (pump_ctrl_set(value) == 0) {
					LOG_INF("Set pump value success");
				}
			} else {
				LOG_ERR("Invalid pump command");
			}
		} else if (strcmp(user_cmd, "sensor") == 0) {
			if (strcmp(user_sub_cmd, "on") == 0) {
				sensor_debug_flag = true;
				LOG_INF("Sensor debug on");
			}
			else if (strcmp(user_sub_cmd, "off") == 0) {
				sensor_debug_flag = false;
				LOG_INF("Sensor debug off");
			} else {
				LOG_ERR("Invalid sensor command");
			}
		// } else if (strcmp(user_cmd, "pressure") == 0) {
		// 	if (strcmp(user_sub_cmd, "on") == 0) {
		// 		sensor_debug_flag = true;
		// 		LOG_INF("Sensor debug on");
		// 	}
		// 	else if (strcmp(user_sub_cmd, "offset=") == 0) {
		// 		sensor_debug_flag = false;
		// 		LOG_INF("Sensor debug off");
		// 	} else {
		// 		LOG_ERR("Invalid sensor command");
		// 	}
		} else if (strcmp(user_cmd, "help") == 0) {
			LOG_INF("Available commands: net, pid, sys");
		} else {
			LOG_ERR("Invalid command");
		}
		memset(data_buff, 0, sizeof(data_buff));
		memset(user_cmd, 0, sizeof(user_cmd));
		memset(user_sub_cmd, 0, sizeof(user_sub_cmd));
		memset(user_sub_action, 0, sizeof(user_sub_action));
	}
}

static struct k_thread uart_handle_thread;
static K_KERNEL_STACK_MEMBER(uart_handle_stack, SHELL_STACK_SIZE);

void uart_thread_init()
{
	k_thread_create(&uart_handle_thread, uart_handle_stack, K_THREAD_STACK_SIZEOF(uart_handle_stack),
					uart_handle, NULL, NULL, NULL, 20, 0,
					K_NO_WAIT);
	return;
}