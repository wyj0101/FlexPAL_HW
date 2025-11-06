/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #ifndef ZEPHYR_DRIVERS_SENSOR_SGM41511_SGM41511_H_
// #define ZEPHYR_DRIVERS_SENSOR_SGM41511_SGM41511_H_
#pragma once

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT sgmicro_sgm41511

#define SGM41511_BUS_SPI DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define SGM41511_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

union sgm41511_bus {
#if SGM41511_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if SGM41511_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*sgm41511_bus_check_fn)(const union sgm41511_bus *bus);
typedef int (*sgm41511_reg_read_fn)(const union sgm41511_bus *bus,
				  uint8_t start, uint8_t *buf, int size);
typedef int (*sgm41511_reg_write_fn)(const union sgm41511_bus *bus,
				   uint8_t reg, uint8_t val);

struct sgm41511_bus_io {
	sgm41511_bus_check_fn check;
	sgm41511_reg_read_fn read;
	sgm41511_reg_write_fn write;
};

#if SGM41511_BUS_SPI
#define SGM41511_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB |	\
			      SPI_MODE_CPOL | SPI_MODE_CPHA)
extern const struct sgm41511_bus_io sgm41511_bus_io_spi;
#endif

#if SGM41511_BUS_I2C
extern const struct sgm41511_bus_io sgm41511_bus_io_i2c;
#endif


enum OTG_CMD {
	OTG_ENABLE,
	OTG_DISABLE,
};

enum BATFET_DLY_SELECT {
	IMMEDIATELY,
	/* 8s 延时 */
	SECONDS_DELAY,
};
enum CHARG_STATUS {
	IS_DIS_CHARGE,
	IS_PRE_CHARGE,
	IS_FAST_CHARGE,
	IS_FULL_CHARGE,
};

enum battary_status {
	BATT_CHARG_NONE = 0,
	BATT_IN_CHARGING,
	BATT_CHARG_VOL_FULL,
};

#define SGM_DEVICE_ADDR 0x6B
/* 0X6B <<1 0xD6 */
#define SGM_WRITE_ADDR ((SGM_DEVICE_ADDR << 1) | 0)
/* (0X6B << 1) + 1 0xD7 */
#define SGM_READ_ADDR ((SGM_DEVICE_ADDR << 1) | 1)

#define SGM41511_REG00_ADDR 0x00
#define SGM41511_REG01_ADDR 0x01
#define SGM41511_REG02_ADDR 0x02
#define SGM41511_REG03_ADDR 0x03
#define SGM41511_REG04_ADDR 0x04
#define SGM41511_REG05_ADDR 0x05
#define SGM41511_REG06_ADDR 0x06
#define SGM41511_REG07_ADDR 0x07
#define SGM41511_REG08_ADDR 0x08
#define SGM41511_REG09_ADDR 0x09
#define SGM41511_REG0A_ADDR 0x0A
#define SGM41511_REG0B_ADDR 0x0B

/* register bits settings */
#define STAT (3 << 5)
#define CURRENT_LIMIT_3A2 (0x1f << 0)
#define CURRENT_LIMIT_1A1 (0x0a << 0)

#define FAST_CHRG_VAL_MASK (0x3f << 0)
#define CURRENT_CHARG_2A (0x22 << 0)
#define CURRENT_CHARG_240MA (0x4 << 0)

#define CLEAR_PRE_CHARGE (0xf << 4)
/* Pre-Charge Current Limit (n: 4 bits):
= 60 + 60n (mA) (n ≤ 12) 
*/
#define PRE_CHARGE (0x00)
#define CLEAR_TERM_CHARGE (0xf << 0)
#define TERM_CHARGE (0x00 << 0)

#define WATCHDOG (3 << 4)
#define SGM41511_FORCE_IINDPM_ENABLE (1 << 7)
#define VINDPM_MASK (0xf << 0)
#define VINDPM_4V4 (0x5 << 0)
#define BOOSTV_5V3 (0x3 << 4)
#define BOOSTV_5V15 (0x2 << 4)

#define REG_RST (1 << 7)
#define BOOST_LIM (1 << 7)

/* this bit can enable charge */
#define CHG_CONFIG (1 << 4)

#define OTG_CONFIG (1 << 5)

#define BATFET_DLY_IMD (1 << 3)
#define BATFET_DIS (1 << 5)

/* 获取充电状态 */
#define MASK_CHARGE_STATUS (3 << 3)
#define CMP_DIS_CHARGE (0)
#define CMP_PRE_CHARGE (1 << 3)
#define CMP_FAST_CHARGE (2 << 3)
#define CMP_FULL_CHARGE (3 << 3)

/**                                                                                            
 * struct i2c_sgm41511_platform_data.                                                          
 */
struct i2c_sgm41511_platform_data {
	/* nINT pin */
	unsigned int gpio_int;
	/* nCE pin */
	unsigned int gpio_ce;
	/* OTG pin */
	unsigned int gpio_otg;
};

enum SGM41511_ICHG {
	ICHG_480MA = (1 << 3),
	ICHG_960MA = (1 << 4),
	ICHG_2040MA = 0x22,
	ICHG_3200MA = 0x1A,
};

int sgm41511_enter_ship_mode(enum BATFET_DLY_SELECT select);
int sgm41511_otg_mode_on_off(bool on_off);
int sgm41511_otg_mode_get(void);
int sgm41511_fast_charge_current(enum SGM41511_ICHG ICHG);
int sgm41511_otg_mode_check_and_reenter(void);
// #endif /* ZEPHYR_DRIVERS_SENSOR_SGM41511_SGM41511_H_ */
