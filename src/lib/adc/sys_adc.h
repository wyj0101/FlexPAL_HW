#ifndef SYS_ADC_H
#define SYS_ADC_H

#include "uart_handle.h"

#define ADC_CHANNEL_BAT 1U
#define ADC_CHANNEL_BUS 0U

int sys_adc_init(void);
uint32_t sys_adc_read(uint8_t channel);

#endif