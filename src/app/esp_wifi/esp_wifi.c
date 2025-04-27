#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/sys/reboot.h>

#include "util.h"
#include "esp_wifi.h"
#include "flash_rw.h"

static const struct device *const esp_dev = DEVICE_DT_GET(DT_ALIAS(uart2));

LOG_MODULE_REGISTER(wifi, LOG_DEBUG);

#define MSG_SIZE 32
/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(wifi_msgq, MSG_SIZE, 5, 4);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;


/*
 * Print a null-terminated string character by character to the UART interface
 */
void esp_wifi_print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++)
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
		printf("%c", rec_data);
		// if ((c == '\n' || c == '\r') && rx_buf_pos > 0)
		// if ((rec_data == '\n') && rx_buf_pos > 0) {
		if (rx_buf_pos > 0) {
			/* terminate string */
			// rx_buf[rx_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&wifi_msgq, &rx_buf, K_NO_WAIT);
			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		}
		else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = rec_data;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

static void wifi_handle(void *arug0, void *arug1, void *arug2)
{
	char data_buff[MSG_SIZE];

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

	LOG_DBG("start wifi uart input");
	/* indefinitely wait for input from the user */
	while (k_msgq_get(&wifi_msgq, &data_buff, K_FOREVER) == 0)
	{
		LOG_DBG("data_buff: %s\n", data_buff);
	}
}

static struct k_thread wifi_handle_thread;
static K_KERNEL_STACK_MEMBER(wifi_handle_stack, WIFI_STACK_SIZE);

void esp_wifi_init()
{
	k_thread_create(&wifi_handle_thread, wifi_handle_stack, K_THREAD_STACK_SIZEOF(wifi_handle_stack),
					wifi_handle, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0,
					K_NO_WAIT);
	return;
}