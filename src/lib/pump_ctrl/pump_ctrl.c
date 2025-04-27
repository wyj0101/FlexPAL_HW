#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

#include "pump_ctrl.h"

static const struct device *pump_dev = DEVICE_DT_GET(DT_NODELABEL(pump));
static const struct device *gpiob_dev = DEVICE_DT_GET(DT_NODELABEL(gpiob));

static void valve_gpio_init(void)
{
	gpio_pin_configure(gpiob_dev, VALVE_1_PORT, GPIO_OUTPUT);
	gpio_pin_configure(gpiob_dev, VALVE_2_PORT, GPIO_OUTPUT);
}
int pump_ctrl_init()
{
	if (!device_is_ready(pump_dev)) {
		printk("pwm dev is no ready\n");
		return -1;
	}
	valve_gpio_init();
	
	return 0;
}
int pump_ctrl_set(float value)
{
	float frequency = value * 0.01;

	if(frequency > 0) {
		gpio_pin_set(gpiob_dev, VALVE_1_PORT, VALVE_OFF);
		gpio_pin_set(gpiob_dev, VALVE_2_PORT, VALVE_OFF);
	} else if(frequency < 0) {
		gpio_pin_set(gpiob_dev, VALVE_1_PORT, VALVE_OFF);
		gpio_pin_set(gpiob_dev, VALVE_2_PORT, VALVE_ON);
	} else {
		gpio_pin_set(gpiob_dev, VALVE_1_PORT, VALVE_OFF);
		gpio_pin_set(gpiob_dev, VALVE_2_PORT, VALVE_OFF);
	}
	return pwm_set_cycles(pump_dev, PUMP_PWM_CHANNEL, PUMP_PERIOD, frequency * PUMP_PERIOD, 0);
}
