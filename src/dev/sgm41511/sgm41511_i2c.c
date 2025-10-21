/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for SGM41511 accessed via I2C.
 */

#include "sgm41511.h"

#if SGM41511_BUS_I2C
static int sgm41511_bus_check_i2c(const union sgm41511_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int sgm41511_reg_read_i2c(const union sgm41511_bus *bus,
			       uint8_t start, uint8_t *buf, int size)
{
	return i2c_burst_read_dt(&bus->i2c, start, buf, size);
}

static int sgm41511_reg_write_i2c(const union sgm41511_bus *bus,
				uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, reg, val);
}

const struct sgm41511_bus_io sgm41511_bus_io_i2c = {
	.check = sgm41511_bus_check_i2c,
	.read = sgm41511_reg_read_i2c,
	.write = sgm41511_reg_write_i2c,
};
#endif /* SGM41511_BUS_I2C */
