/* sgm41511.c - Driver for Bosch SGM41511 temperature and pressure sensor */

/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <complex.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
// #include "kbd_events.h"
// #include "event_list.h"

#if CONFIG_LOG
#include <zephyr/logging/log.h>
#endif
#include "logging.h"

#include "sgm41511.h"

// 注册日志模块
LOG_MODULE_REGISTER(SGM41511, LOG_DEBUG);
// 全局设备指针，用于在其他模块中访问SGM41511
const static struct device *g_dev_smg41511;

// 定义是否使用中断功能（注释掉表示不使用）
#define SGM41511_USE_INT

// 宏定义：将枚举值转换为字符串
#define DEFINE_STR(R) #R
// 充电状态字符串数组，用于日志输出
static const char *charge_state_str[] = {
	DEFINE_STR(IS_DIS_CHARGE),   // 放电状态
	DEFINE_STR(IS_PRE_CHARGE),   // 预充电状态
	DEFINE_STR(IS_FAST_CHARGE),  // 快速充电状态
	DEFINE_STR(IS_FULL_CHARGE),  // 充满状态
};

// 检查设备树中是否有可用的SGM41511设备实例
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "SGM41511 driver enabled without any devices"
#endif

// 数据结构：存储驱动实例的动态数据


struct sgm41511_data {
	const struct device *dev;           // 设备指针
	struct gpio_callback callback;      // GPIO回调函数
	struct k_work_delayable dwork;      // 延迟工作队列，用于轮询或延时处理
	enum CHARG_STATUS charge_state;     // 当前充电状态
};

// 配置结构：存储设备树中的静态配置
struct sgm41511_config {
	union sgm41511_bus bus;             // 总线联合体（I2C或SPI）

	const struct sgm41511_bus_io *bus_io; // 总线操作函数指针
	struct gpio_dt_spec ce_gpio;        // 充电使能GPIO（Active Low）

	struct gpio_dt_spec intr_gpio;      // 中断GPIO
	/** 
	 * Power Source Selection Input. 
	 * If PSEL is pulled high, 
	 * the input current limit is set to 500mA (USB 2.0) 
	 * and if it is pulled low, the limit is set to 2.4A (adaptor). 
	 * use input current limit value by writing to the IINDPM[4:0] register.
	*/
	struct gpio_dt_spec psel_gpio;      // 电源选择GPIO
};

// 总线检查函数
static inline int sgm41511_bus_check(const struct device *dev)
{
	const struct sgm41511_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

// 寄存器读取函数
static inline int sgm41511_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct sgm41511_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, start, buf, size);
}

// 寄存器写入函数
static inline int sgm41511_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct sgm41511_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, val);
}

// 寄存器查看函数：以二进制格式打印所有寄存器值（用于调试）
static int sgm41511_register_view(const struct device *dev)
{
	int err;
	char c[9] = { 0 };      // 8位二进制字符串 + 结束符

	uint8_t reg_val;        // 寄存器值
	uint8_t val = 0;
	int8_t i = 0, j = 0;

	/* 遍历所有寄存器（0x00到0x0B） */

	for (i = 0; i < (SGM41511_REG0B_ADDR + 1); i++) {
		// 读取寄存器值
		err = sgm41511_reg_read(dev, i, &reg_val, sizeof(val));
		if (err < 0)
			return -EIO;

		// 将寄存器值转换为二进制字符串
		for (j = ((sizeof(reg_val) * 8) - 1); j >= 0; j--)
			c[7 - j] = (!!((1 << j) & reg_val) + 0x30);  // 将每位转换为'0'或'1'

		c[8] = '\0';
		LOG_DBG("SGM41511-REG_%d (format-binary): %s\n", i, c);
	}
	return 0;
}

// 获取充电状态函数
static int sgm41511_get_status(const struct device *dev)
{
	int err;
	static uint8_t charge_status = 0, last_charge_status = 0;  // 静态变量保存上次状态
	struct sgm41511_data *data = dev->data;
	// enum battary_status batt_level = BATT_CHARG_NONE;          // 电池状态

	// 读取状态寄存器（0x08）
	err = sgm41511_reg_read(dev, SGM41511_REG08_ADDR, &charge_status, sizeof(charge_status));
	if (err < 0) {
		LOG_ERR("%s %d Failed to get sgm41511 charge state: %d !\n", __func__, __LINE__,
			err);
	} else {
		// 检查充电状态是否发生变化
		if ((charge_status & MASK_CHARGE_STATUS) != last_charge_status) {
			last_charge_status = (charge_status & MASK_CHARGE_STATUS);
			
			/* 根据充电状态位判断具体状态 */
			switch (charge_status & MASK_CHARGE_STATUS) {
			case CMP_PRE_CHARGE:
				data->charge_state = IS_PRE_CHARGE;
#if defined(CONFIG_RGBM_BLINK_ENABLE) && defined(CONFIG_RGBCTRL_ENABLE)
				// 如果配置了RGB控制，发送电池充电状态事件
				batt_level = BATT_IN_CHARGING;
				QMK_EVT_POST(EVT_TYPE_UPDATE_STA, BATT_CHRG_STATUS, batt_status,
					batt_level);
#endif
				break;
			case CMP_FAST_CHARGE:
				data->charge_state = IS_FAST_CHARGE;
#if defined(CONFIG_RGBM_BLINK_ENABLE) && defined(CONFIG_RGBCTRL_ENABLE)
				batt_level = BATT_IN_CHARGING;
				QMK_EVT_POST(EVT_TYPE_UPDATE_STA, BATT_CHRG_STATUS, batt_status,
					batt_level);
#endif
				break;
			case CMP_FULL_CHARGE:
				data->charge_state = IS_FULL_CHARGE;
#if defined(CONFIG_RGBM_BLINK_ENABLE) && defined(CONFIG_RGBCTRL_ENABLE)
				batt_level = BATT_CHARG_VOL_FULL;
				QMK_EVT_POST(EVT_TYPE_UPDATE_STA, BATT_CHRG_STATUS, batt_status,
					batt_level);
#endif
				break;
			case CMP_DIS_CHARGE:
				data->charge_state = IS_DIS_CHARGE;
#if defined(CONFIG_RGBM_BLINK_ENABLE) && defined(CONFIG_RGBCTRL_ENABLE)
				batt_level = BATT_CHARG_NONE;
				QMK_EVT_POST(EVT_TYPE_UPDATE_STA, BATT_CHRG_STATUS, batt_status,
					batt_level);
#endif
				break;
			default:
				return -EAGAIN;  // 未知状态，需要重试
			}

			LOG_DBG("current charge state: %s", charge_state_str[data->charge_state]);
			err = 0;
		}
	}

	return err;
}

// 充电状态更新函数
static void charge_status_update(const struct device *dev)
{
	// TODO: Charge Enable Input Pin (Active Low). Battery charging is enabled when CHG_CONFIG bit is 1 and nCE
	//pin is pulled low.
	LOG_DBG("\"%s\" charge_status_update", dev->name);
	// led_on(led_device, 3);  // 可以控制LED指示状态
	sgm41511_get_status(dev);  // 获取当前充电状态
	// sgm41511_register_view(dev);  // 调试用：查看寄存器
}

// OTG模式开关控制函数
int sgm41511_otg_mode_on_off(bool on_off)
{
	int err;
	uint8_t val;
	if (!g_dev_smg41511)
		LOG_ERR("sgm41511 global pointer not set");

	// 读取寄存器0x01
	err = sgm41511_reg_read(g_dev_smg41511, SGM41511_REG01_ADDR, &val, sizeof(val));
	if (err < 0)
		return err;

	// 设置或清除OTG配置位
	if (on_off)
		val |= OTG_CONFIG;      // 启用OTG
	else
		val &= (~OTG_CONFIG);   // 禁用OTG

	// 写回寄存器
	err = sgm41511_reg_write(g_dev_smg41511, SGM41511_REG01_ADDR, val);
	if (err)
		return err;

	return 0;
}

// 获取OTG模式状态
int sgm41511_otg_mode_get(void)
{
	int err;
	uint8_t val;
	if (!g_dev_smg41511)
		LOG_ERR("sgm41511 global pointer not set");

	// 读取寄存器0x01
	err = sgm41511_reg_read(g_dev_smg41511, SGM41511_REG01_ADDR, &val, sizeof(val));
	if (err < 0) {
		LOG_ERR("%s %d Failed to get sgm41511 val: %d !\n", __func__, __LINE__, err);
		return err;
	}

	// 返回OTG配置位状态
	return val & OTG_CONFIG;
}

// 进入运输模式函数（低功耗模式）
int sgm41511_enter_ship_mode(enum BATFET_DLY_SELECT select)
{
	uint8_t ret;
	uint8_t val;

	if (!g_dev_smg41511)
		LOG_ERR("sgm41511 global pointer not set");

	// 读取寄存器0x07
	ret = sgm41511_reg_read(g_dev_smg41511, SGM41511_REG07_ADDR, &val, sizeof(val));
	if (ret < 0) {
		LOG_ERR("%s %d Failed to get sgm41511 val: %d !\n", __func__, __LINE__, ret);
		return ret;
	}

	/* 设置延迟时间 */

	if (SECONDS_DELAY == select)
		val |= BATFET_DLY_IMD;  // 8秒后进入运输模式
	else
		val &= ~BATFET_DLY_IMD; // 立即进入运输模式

	val |= BATFET_DIS;  // 禁用BATFET，进入运输模式

	// 写回寄存器
	return sgm41511_reg_write(g_dev_smg41511, SGM41511_REG07_ADDR, val);
}

// 延迟工作队列回调函数：定期更新充电状态
static void sgm41511_cooldown_expired(struct k_work *work)
{
	// struct sgm41511_data *data;
	// data = CONTAINER_OF(work, struct sgm41511_data, dwork);  // 获取包含工作队列的数据结构

	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct sgm41511_data *data = CONTAINER_OF(dwork, struct sgm41511_data, dwork);

	k_sleep(K_MSEC(10));  // 短暂延迟
	charge_status_update(data->dev);  // 更新充电状态
	// k_work_reschedule(&data->dwork, K_MSEC(1000));  // 重新调度，1秒后再次执行
}

#if defined(SGM41511_USE_INT)
// 中断消抖处理函数
static void debounce_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	const struct sgm41511_config *cfg;
	struct sgm41511_data *data;

	data = CONTAINER_OF(cb, struct sgm41511_data, callback);  // 获取包含回调的数据结构
	cfg = data->dev->config;

	/**
	 * @brief Open-Drain Interrupt Output Pin.
	 * Use a 10kΩ pull-up to the logic high rail.
	 * The nINT pin sends out a negative 256μs pulse
	 * to the host when a fault occurs or a new charge status updates.
	 */
	// 检查中断是否来自SGM41511的中断引脚
	if (cfg->intr_gpio.port == port && (BIT(cfg->intr_gpio.pin) == pins)) {
		k_work_reschedule(&data->dwork, K_USEC(10));  // 10微秒后执行工作队列
	}
}

// 引脚中断使能函数
static int sgm41511_pin_interrupt_enable(const struct device *dev)
{
       int err;
       gpio_flags_t flags;
       bool is_active_low;
       const struct sgm41511_config *cfg = dev->config;
       const struct gpio_dt_spec *gpio = &cfg->intr_gpio;

       // 检查是否是低电平有效
       is_active_low = gpio->dt_flags & GPIO_ACTIVE_LOW;
       if (is_active_low) {
               LOG_INF("active_low");
               flags = GPIO_INT_EDGE_FALLING & ~GPIO_INT_DISABLE;  // 下降沿触发
       } else {
               LOG_INF("active_high");
               flags = GPIO_INT_EDGE_RISING & ~GPIO_INT_DISABLE;   // 上升沿触发
       }

       // 配置引脚中断
       err = gpio_pin_interrupt_configure_dt(gpio, flags);
       if (err) {
               LOG_ERR("Unable to configure interrupt for pin %u on %s\n", gpio->pin,
                       gpio->port->name);
               return err;
       }
       return 0;
}
#endif

// 控制引脚初始化函数
static int sgm41511_ctrl_pins_init(const struct device *dev)
{
	int err;
	const struct sgm41511_config *cfg = dev->config;
	struct sgm41511_data *data = dev->data;
	data->dev = dev;

	LOG_DBG("sgm41511 ctrl pins init");

 	/* nCE引脚配置（充电使能，低电平有效） */
 	if (cfg->ce_gpio.port != NULL) {
		/* nCE gpio set low */
		if (!gpio_is_ready_dt(&cfg->ce_gpio)) {
			LOG_ERR("%s: gpio pin: %s not ready", dev->name, cfg->ce_gpio.port->name);
			return -ENODEV;
		}

		// 配置为输出，初始化为低电平
		err = gpio_pin_configure_dt(&cfg->ce_gpio, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
		if (err < 0) {
			LOG_ERR("%s: can't configure nCE pin (%d) as output", dev->name, cfg->ce_gpio.pin);
			return err;
		}

		// 设置引脚为高电平（由于是低电平有效，这会禁用充电？需要确认逻辑）
		err = gpio_pin_set_dt(&cfg->ce_gpio, true);
		if (err < 0) {
			LOG_ERR("Could not set nCE GPIO val (%d)", err);
			return err;
		}
	}
	
	/* PSEL引脚配置（电源选择） */
	/* PSEL gpio set low to use input current limit value by writing to the IINDPM[4:0] register.*/
	if (cfg->psel_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->psel_gpio)) {
			LOG_ERR("%s: gpio pin: %s not ready", dev->name, cfg->psel_gpio.port->name);
			return -ENODEV;
		}

		// 配置为输出，初始化为低电平
		err = gpio_pin_configure_dt(&cfg->psel_gpio, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
		if (err < 0) {
			LOG_ERR("%s: can't configure PSEL pin (%d) as output", dev->name, cfg->psel_gpio.pin);
			return err;
		}

		err = gpio_pin_set_dt(&cfg->psel_gpio, true);
		if (err < 0) {
			LOG_ERR("Could not set PSEL GPIO val (%d)", err);
			return err;
		}
	}
	
#if defined(SGM41511_USE_INT)
	/* nINT引脚配置（中断输入） */
	if (!gpio_is_ready_dt(&cfg->intr_gpio)) {
		LOG_ERR("%s: gpio pin: %s not ready", dev->name, cfg->intr_gpio.port->name);
		return -ENODEV;
	}

	// 配置为输入
	err = gpio_pin_configure_dt(&cfg->intr_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("%s: can't configure intr pin (%d) as input", dev->name,
			cfg->intr_gpio.pin);
		return err;
	}

	/* nINT gpio interrupt setting */
	// 初始化GPIO回调
	gpio_init_callback(&data->callback, debounce_handler, BIT(cfg->intr_gpio.pin));
	// 添加回调函数
	err = gpio_add_callback(cfg->intr_gpio.port, &data->callback);
	if (err) {
		LOG_ERR("Error adding the callback to the input switch device: %i", err);
		return err;
	}
#endif


	/* 延迟工作队列初始化，用于信号消抖 */
	k_work_init_delayable(&data->dwork, sgm41511_cooldown_expired);

// 如果不使用中断，则使用轮询方式定期检查状态
#ifndef SGM41511_USE_INT
	k_work_reschedule(&data->dwork, K_MSEC(1000));  // 1秒后开始轮询
#endif

	return 0;
}

// 寄存器配置函数：初始化芯片寄存器
static int sgm41511_reg_configure(const struct device *dev)
{
	int err;
	uint8_t val = 0;

	// 软件复位
	err = sgm41511_reg_read(dev, SGM41511_REG0B_ADDR, &val, sizeof(val));
	if (err < 0) {
		LOG_DBG("read REG00 failed: %d", err);
		return err;
	}

	err = sgm41511_reg_write(dev, SGM41511_REG0B_ADDR, (val | REG_RST));
	if (err < 0) {
		LOG_DBG("Soft-reset failed: %d", err);
	}

	/* 重置所有寄存器为默认值 */

	err = sgm41511_reg_read(dev, SGM41511_REG0B_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val |= REG_RST;  // 设置复位位
	err = sgm41511_reg_write(dev, SGM41511_REG0B_ADDR, val);
	if (err)
		goto out;

	/*
	 * 防止看门狗复位导致设备返回默认模式
	 * 主机必须通过设置 WATCHDOG[1:0] = 00 禁用看门狗定时器，
	 * 或者在到期前通过向 WD_RST 写入 1 来持续重置看门狗定时器
	 * 以防止设置 WATCHDOG_FAULT 位。
	 * To prevent device watchdog reset
	 * that results in going back to default mode, the host must
	 * either disable the watchdog timer by setting WATCHDOG[1:0] = 00, 
	 * or it must consistently reset the watchdog timer before
	 * expiry by writing 1 to WD_RST to prevent WATCHDOG_FAULT
	 * bit to be set.
	 */
	err = sgm41511_reg_read(dev, SGM41511_REG05_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val &= (~WATCHDOG);  // 禁用看门狗
	err = sgm41511_reg_write(dev, SGM41511_REG05_ADDR, val);
	if (err)
		goto out;

	err = sgm41511_reg_read(dev, SGM41511_REG05_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;
	LOG_DBG("#### REG05_ADDR: 0x%x ####", val);

	/* 使能输入电流限制检测 */

	err = sgm41511_reg_read(dev, SGM41511_REG07_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val |= SGM41511_FORCE_IINDPM_ENABLE;  // 强制IINDPM使能,当vbus存在时，强制使用进行输入电流限制检测
	err = sgm41511_reg_write(dev, SGM41511_REG07_ADDR, val);
	if (err)
		goto out;

	/* VINDPM设置：4.4V输入电压限制 */
	err = sgm41511_reg_read(dev, SGM41511_REG06_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val &= (~VINDPM_MASK);  // 清除VINDPM位 清空第四位
	val |= VINDPM_4V4;      // 设置4.4V为输入的跌落电压下限阈值，默认输入电压上限为6.5V
	val |= BOOSTV_5V3;     // 设置输出升压电压为5.3V
	err = sgm41511_reg_write(dev, SGM41511_REG06_ADDR, val);
	if (err)
		goto out;

	/* disable SATA */
	/* restrict charge current */
	err = sgm41511_reg_read(dev, SGM41511_REG00_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	// 默认打开stat引脚功能
	// val &= (~STAT);              // 清除STAT位
	// val |= STAT;                 // 设置STAT位（具体功能需查手册）


	val &= (~CURRENT_LIMIT_3A2); // 清除电流限制位
	val |= CURRENT_LIMIT_1A1;    // 设置电流限制为1.1A
	err = sgm41511_reg_write(dev, SGM41511_REG00_ADDR, val);
	if (err)
		goto out;

	/* 预充电电流(60mA)和终止电流(180mA)设置 */

	err = sgm41511_reg_read(dev, SGM41511_REG03_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val &= (~CLEAR_PRE_CHARGE);  // 清除预充电位
	val |= PRE_CHARGE;           // 设置预充电电流 60MA

	val &= (~CLEAR_TERM_CHARGE); // 清除终止充电位
	val |= TERM_CHARGE;          // 设置终止充电电流 180ma
	err = sgm41511_reg_write(dev, SGM41511_REG03_ADDR, val);
	if (err)
		goto out;

	/* 充电电流设置和升压限制到240MA */

	err = sgm41511_reg_read(dev, SGM41511_REG02_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val &= (~FAST_CHRG_VAL_MASK); // 清除快速充电位
	val |= CURRENT_CHARG_240MA;      // 设置充电电流为240MA
	val |= BOOST_LIM;             // 设置升压限制

	err = sgm41511_reg_write(dev, SGM41511_REG02_ADDR, val);
	if (err)
		goto out;

	/* 充电电压限制到4.352V */

	err = sgm41511_reg_read(dev, SGM41511_REG04_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val &= ~0xF8;        // 清除电压设置位

	val |= (0xb << 3);   // 设置充电电压为4.208V
	err = sgm41511_reg_write(dev, SGM41511_REG04_ADDR, val);
	if (err)
		goto out;

	/* 充电使能 */

	err = sgm41511_reg_read(dev, SGM41511_REG01_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val &= (~CHG_CONFIG);  // 清除充电配置位
	val |= CHG_CONFIG;     // 使能充电
	// val |= OTG_CONFIG;  // 使能OTG功能
	err = sgm41511_reg_write(dev, SGM41511_REG01_ADDR, val);
	if (err)
		goto out;

	return 0;

out:
	LOG_DBG("\"%s\" register config failed", dev->name);
	return err;
}

// 快速充电电流设置函数
int sgm41511_fast_charge_current(enum SGM41511_ICHG ICHG)
{
	int err;
	uint8_t val = 0;

	if (!g_dev_smg41511)
		LOG_ERR("sgm41511 global pointer not set");

	/* 充电电流设置和升压限制到1.2A */

	err = sgm41511_reg_read(g_dev_smg41511, SGM41511_REG02_ADDR, &val, sizeof(val));
	if (err < 0)
		goto out;

	val &= (~FAST_CHRG_VAL_MASK);  // 清除快速充电位
	val |= ICHG;                   // 设置指定的充电电流

	err = sgm41511_reg_write(g_dev_smg41511, SGM41511_REG02_ADDR, val);
	if (err)
		goto out;

	return 0;
	
out:
	LOG_DBG("fast_charge_current config failed");
	return err;
}

// 芯片初始化函数
static int sgm41511_chip_init(const struct device *dev)
{
	int err;
	// struct sgm41511_data *data = dev->data;
	g_dev_smg41511 = dev;  // 设置全局设备指针

	LOG_DBG("sgm41511_chip_init");

	// 初始化控制引脚
	err = sgm41511_ctrl_pins_init(dev);
	if (err) {
		LOG_ERR("%s: can't configure ctrl pins", dev->name);
		return err;
	}

	// 检查总线是否就绪
	err = sgm41511_bus_check(dev);
	if (err < 0) {
		LOG_DBG("bus check failed: %d", err);
		return err;
	}

	// 配置寄存器
	err = sgm41511_reg_configure(dev);
	if (err < 0) {
		LOG_ERR("%s: can't configure internal registers", dev->name);
		return err;
	}

	/* 等待传感器就绪 */

	k_sleep(K_MSEC(1));

	// 查看寄存器状态（调试用）
	err = sgm41511_register_view(dev);
	if (err)
		return err;
		
#if defined(SGM41511_USE_INT)
	/* nINT gpio interrupt enable */
	// 使能引脚中断
	err = sgm41511_pin_interrupt_enable(dev);
	if (err) {
		LOG_ERR("Error sgm41511 pin interrupt enable failed: %i", err);
		return err;
	}
#endif

	// 防止中断配置过程中错过一次充电状态更新的中断
	sgm41511_get_status(dev);

	LOG_DBG("\"%s\" OK", dev->name);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
// 电源管理动作处理函数
static int sgm41511_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* 恢复设备：重新初始化芯片 */
		ret = sgm41511_chip_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* 挂起设备：可以添加低功耗处理 */
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/* 为SPI总线实例初始化sgm41511_config结构 */

#define SGM41511_CONFIG_SPI(inst)                                                                  \
	{                                                                                          \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst, SGM41511_SPI_OPERATION, 0),                  \
		.bus_io = &sgm41511_bus_io_spi,                                                    \
		.ce_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ce_gpios, {0}),                                  \
		.intr_gpio = GPIO_DT_SPEC_INST_GET(inst, intr_gpios),                              \
		.psel_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, psel_gpios, {0}),                      \
	}

/* 为I2C总线实例初始化sgm41511_config结构 */

#define SGM41511_CONFIG_I2C(inst)                                                                  \
	{                                                                                          \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.bus_io = &sgm41511_bus_io_i2c,             \
		.ce_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ce_gpios, {0}),                          \
		.intr_gpio = GPIO_DT_SPEC_INST_GET(inst, intr_gpios),                              \
		.psel_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, psel_gpios, {0}),                      \
	}

/*
 * 主实例化宏，为实例选择正确的总线特定实例化宏


 */
#define SGM41511_DEFINE(inst)                                                                      \
	static struct sgm41511_data sgm41511_data_##inst;                                          \
	static const struct sgm41511_config sgm41511_config_##inst =                               \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (SGM41511_CONFIG_SPI(inst)),                \
			    (SGM41511_CONFIG_I2C(inst)));                                          \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, sgm41511_pm_action);                                        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sgm41511_chip_init, PM_DEVICE_DT_INST_GET(inst),        \
				     &sgm41511_data_##inst, &sgm41511_config_##inst, POST_KERNEL,  \
				     95, NULL);

/* 为设备树中每个状态为"okay"的节点创建设备结构 */

DT_INST_FOREACH_STATUS_OKAY(SGM41511_DEFINE)

#if 0
// 重新定义字符串转换宏（前面已定义，这里可能是重复的）
#define DEFINE_STR(R)  #R

// USB状态字符串数组
static const char* USB_STATUS_STR[] = {
	DEFINE_STR(KBD_USB_STATE_DISCONNECTED),  // USB断开连接
	DEFINE_STR(KBD_USB_STATE_POWERED),       // USB供电但未枚举
	DEFINE_STR(KBD_USB_STATE_ACTIVE),        // USB激活状态
	DEFINE_STR(KBD_USB_STATE_SUSPENDED),     // USB挂起状态
	DEFINE_STR(KBD_USB_STATE_ERROR)          // USB错误状态
	};

// USB自动充电回调函数

static void callback_listener_sgm41511_usb_auto_charging(struct kbd_msg *msg)
{
	// 检查事件类型和代码
	if (msg->header.type == EVT_TYPE_UPDATE_STA && msg->header.code == KBD_USB_STATUS) {
		// 根据USB状态设置不同的充电电流
		switch (msg->msg.usb_status) {

		case KBD_USB_STATE_ACTIVE:
			// USB激活状态：使用小电流480mA，避免影响数据传输
			sgm41511_fast_charge_current(ICHG_480MA);
			LOG_DBG("%s, ICHG: 480MA", USB_STATUS_STR[msg->msg.usb_status]);
			break;
		case KBD_USB_STATE_POWERED:
		case KBD_USB_STATE_SUSPENDED:
			// USB供电或挂起状态：使用大电流2040mA快速充电
			sgm41511_fast_charge_current(ICHG_2040MA);
			LOG_DBG("%s, ICHG: 3200MA", USB_STATUS_STR[msg->msg.usb_status]);
			break;

		default:
			// 其他状态：使用默认小电流480mA
			sgm41511_fast_charge_current(ICHG_480MA);
			LOG_DBG("%s, ICHG-Default: 480MA", USB_STATUS_STR[msg->msg.usb_status]);
			break;
		}
	}
}

// 订阅USB状态变化事件，当USB状态改变时自动调整充电电流
QMK_EVT_SUBSCRIPTION(listener_usb_auto_charging, EVT_TYPE_UPDATE_STA, KBD_USB_STATUS, callback_listener_sgm41511_usb_auto_charging);
#endif