#pragma once

#define PUMP_PERIOD 3600 // 20Khz
#define PUMP_PWM_CHANNEL 2

#define VALVE_1_PORT 6
#define VALVE_2_PORT 8

enum valve_state {
	VALVE_OFF,
	VALVE_ON
};
/**
 * @brief init pump control module
 * 
 * @return int 
 */
int pump_ctrl_init(void);

/**
 * @brief set pump value
 * 
 * @param value Range 0-100, both positive and negative
 * @return int 
 */
int pump_ctrl_set(float value);