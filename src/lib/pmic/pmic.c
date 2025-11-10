#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/gpio.h>
#include "pmic.h"
#include "logging.h"
#include "sgm41511.h"
#if CONFIG_LOG
#include <zephyr/logging/log.h>
#endif

#define BUTTON_PORT 0
// static const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(pmic));
static const struct device *gpioa_dev = DEVICE_DT_GET(DT_NODELABEL(gpioa));

static struct gpio_callback power_ctl_button_callback;

static struct {
    struct gpio_callback cb;
    struct k_work work;
    int64_t press_time;
    bool pressed;
} button_data;

LOG_MODULE_REGISTER(pmic, LOG_DEBUG);

// 工作队列处理函数
static void button_work_handler(struct k_work *work)
{
    int64_t press_duration = k_uptime_get() - button_data.press_time;
    
    LOG_INF("Button pressed for %lld ms", press_duration);
    
    if (press_duration >= 100 && press_duration < 1000) {
        LOG_INF("Short press - exit otg mode");
        sgm41511_otg_mode_on_off(false);  // 退出OTG模式
        sgm41511_otg_mode_check_and_reenter(); // 防止误触，检查并重新进入OTG模式
    } else if (press_duration >= 3000) {
        // 长按3秒：进入运输模式
        LOG_INF("Long press - Enter shipping mode");
        sgm41511_enter_ship_mode(IMMEDIATELY);
    }
}

void button_irq_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	int val = gpio_pin_get(gpioa_dev, BUTTON_PORT);
    
    if (val == 0) {
        // 按键按下（下降沿）
        button_data.pressed = true;
        button_data.press_time = k_uptime_get();
    } else {
        // 按键释放（上升沿）
        if (button_data.pressed) {
            button_data.pressed = false;
            // 提交到工作队列处理
            k_work_submit(&button_data.work);
        }
    }
}

void button_gpio_init(void)
{
    gpio_pin_configure(gpioa_dev, BUTTON_PORT, GPIO_INPUT);
	gpio_pin_interrupt_configure(gpioa_dev, BUTTON_PORT, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&power_ctl_button_callback, button_irq_handler, BIT(BUTTON_PORT));
	gpio_add_callback(gpioa_dev, &power_ctl_button_callback);

    // 初始化工作队列
    k_work_init(&button_data.work, button_work_handler);
}

int pmic_init(void)
{
    button_gpio_init();
	return 0;
}
