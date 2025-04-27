#include <stdio.h>

#include <zephyr/kernel.h>

#include "uart_handle.h"

int main(void)
{
	uart_thread_init();

	// while (1) {
	// 	printk("Hello, world!\n");
	// 	k_sleep(K_MSEC(1000));
	// }
	return 0;
}