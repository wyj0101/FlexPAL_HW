#pragma once

/************************************
* Version: V1.0
* 两个bus总线的LDC1614读写函数，
* 需要注意LDC1614的寄存器是2个字节，16位的，读写中，先读写高八位，再读写低八位，
* 所以读写dataBuff数组中，高八位存储在dataBuff[0]中，低八位存储在dataBuff[1]中
*
**************************************/
#include <stdint.h>


/*Register Rddr*/
#define LDC_REG_CONVERTION_RESULT_REG_START             0X00
#define LDC_REG_SET_CONVERSION_TIME_REG_START           0X08
#define LDC_REG_SET_CONVERSION_OFFSET_REG_START         0X0C
#define LDC_REG_SET_LC_STABILIZE_REG_START              0X10
#define LDC_REG_SET_FREQ_REG_START                      0X14

#define LDC_REG_SENSOR_STATUS_REG                       0X18
#define LDC_REG_ERROR_CONFIG_REG                        0X19
#define LDC_REG_SENSOR_CONFIG_REG                       0X1A
#define LDC_REG_MUL_CONFIG_REG                          0X1B
#define LDC_REG_SENSOR_RESET_REG                        0X1C
#define LDC_REG_SET_DRIVER_CURRENT_REG                  0X1E

#define LDC_REG_READ_MANUFACTURER_ID                    0X7E
#define LDC_REG_READ_DEVICE_ID                          0X7F

/** 
 * @brief: set conversion time register 
 * 0x0000 - 0x0004 : reserved
 * 0x0005 - 0xffff : conversion time = (RCOUNTx * 16) / fREFx
 * @reset: 0x0080
 * @reg: RCOUNTx
 * @reg_addr: 0x08
 */
#define LDC_RECOUNT_VALUE(x)   ((uint16_t) (x))

/** 
 * @brief: set data offset 
 * fOFFSETx = (OFFSETx / (2 ^ 16)) * fREFx
 * @reset: 0x0000
 * @reg: OFFSETx
 * @reg_addr: 0x0C
 */
#define LDC_OFFSET_VALUE(x)   ((uint16_t) (x))

/** 
 * @brief: set settling time
 * 0x0000: settle time = 32 / fREFx
 * 0x0001: settle time = 32 / fREFx
 * 0x0002 - 0xffff: settle time = (SETTLECOUNTx * 16) / fREFx
 * @reset: 0x0000
 * @reg: SETTLECOUNTx
 * @reg_addr: 0x10
 */
#define LDC_SETTLECOUNT_VALUE(x)   ((uint16_t) (x))

/** 
 * @brief: set clock dividers
 * FIN_DIVIDERx[15:12]: set divider for sensor fre
 * FREF_DIVIDERx[9:0]: set divider for reference clock
 * @reg: CLOCK_DIVIDERSx
 * @reg_addr: 0x14
 */
#define LDC_CLOCK_DIVIDERS_FIN(x) ((uint16_t) ((x) << 12))
#define LDC_CLOCK_DIVIDERS_FREF(x) ((uint16_t) ((x) << 0))

/** 
 * @brief: only read sensor status register
 * @reg: STATUE
 * @reg_addr: 0x18
 */

/** 
 * @brief: config error whether enable
 * @reg: ERROR_CONFIG
 * @reg_addr: 0x19
 */
#define LDC_ERROR_CONFIG_UR_EN         ((uint16_t) (1 << 15))   // under range error
#define LDC_ERROR_CONFIG_OR_EN         ((uint16_t) (1 << 14))   // over range error
#define LDC_ERROR_CONFIG_WD_EN         ((uint16_t) (1 << 13))   // watchdog error
#define LDC_ERROR_CONFIG_AH_EN         ((uint16_t) (1 << 12))   // amplitude high error
#define LDC_ERROR_CONFIG_AL_EN         ((uint16_t) (1 << 11))   // amplitude low error
#define LDC_ERROR_CONFIG_UR_IN_EN      ((uint16_t) (1 << 7))    // under range interrupt enable
#define LDC_ERROR_CONFIG_OR_IN_EN      ((uint16_t) (1 << 6))    // over range interrupt enable
#define LDC_ERROR_CONFIG_WD_IN_EN      ((uint16_t) (1 << 5))    // watchdog interrupt enable
#define LDC_ERROR_CONFIG_AH_IN_EN      ((uint16_t) (1 << 4))    // amplitude high interrupt enable
#define LDC_ERROR_CONFIG_AL_IN_EN      ((uint16_t) (1 << 3))    // amplitude low interrupt enable
#define LDC_ERROR_CONFIG_ZC_IN_EN      ((uint16_t) (1 << 2))    // zero count interrupt enable
#define LDC_ERROR_CONFIG_DRDY_IN_EN    ((uint16_t) (1 << 0))    // data ready interrupt enable

/** 
 * @brief: config chip
 * @reg: CONFIG
 * @reg_addr: 0x1a
 */
#define LDC_CONFIG_ACTIVE_CHAN(x)       ((uint16_t) ((x) << 14))   // single channel selection
#define LDC_CONFIG_SLEEP_MODE_EN        ((uint16_t) (1 << 13))     // sleep mode enable
#define LDC_CONFIG_RP_OVERRIDE_EN       ((uint16_t)(1<<12))   // RP override enable
#define LDC_CONFIG_SENSOR_ACTIVATE_SEL  ((uint16_t)(1<<11))   // low power activate
#define LDC_CONFIG_AUTO_AMP_DIS         ((uint16_t)(1<<10))   // automatic amplitude disable
#define LDC_CONFIG_REF_CLK_SRC          ((uint16_t)(0<<9))    // user internal oscillator
#define LDC_CONFIG_INTB_DIS             ((uint16_t)(1<<7))    // INTB disable
#define LDC_CONFIG_HIGH_CURRENT_DRV     ((uint16_t)(1<<6))    // channel0 > 1.5mA drive

/** 
 * @brief: set chip mux config register
 * @reg: MUX_CONFIG
 * @reg_addr: 0x1B
 */
#define LDC_MUX_CONFIG_AUTOSCAN_EN      ((uint16_t) (1 << 15))      // auto scan enable
#define LDC_MUX_CONFIG_RR_SEQUENCE(x)   ((uint16_t) ((x) << 13))    // RR sequence 00-ch0,1 01-ch0,1,c2 10-ch0,1,2,3
#define LDC_MUX_CONFIG_DEGLITCH(x)      ((uint16_t) ((x) << 0))     // 1-1Mhz 4-3.3Mhz 5-10Mhz 7-33Mhz 

/** 
 * @brief: reset chip
 * @reg: RESET_DEV
 * @reg_addr: 0x1C
 */
#define LDC_RESET_DEV_RESET             ((uint16_t) (1 << 15))

/** 
 * @brief: set driver current
 * @reg: DRIVE_CURRENT
 * @reg_addr: 0x1E
 */
#define LDC_DRIVE_CURRENT_IDRIVE(x)       ((uint16_t) ((x) << 11))

/** 
 * @brief: chip manufacturer id
 * @reg: MANUFACTURER_ID
 * @reg_addr: 0x7E
 * @default: 0x5449  read_only
 */

/** 
 * @brief: chip device id
 * @reg: DEVICE_ID
 * @reg_addr: 0x7F
 * @default: 0x3055  read_only
 */
#define LDC161X_DEVICE_ID                  ((uint16_t) (0x3055))   // 0x3055

#define CONFIG_LDC_CHANNEL_NUM  2

#define CHANNEL_0  0
#define CHANNEL_1  1
#define CHANNEL_2  2
#define CHANNEL_3  3

/*LDC1614函数*/
int LDC161x_init(void);
//结果读取、处理函数
int LDC161x_read_value(uint8_t channel, uint32_t *value);
int LDC161X_auto_calibration(void);