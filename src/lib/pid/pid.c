#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>


// #include "sensor.h"
#include "pid.h"
#include "flash_rw.h"
/*	1- error
 *	2- warning
 *	3- info	(default)
 *	4- debug
 */
// LOG_MODULE_REGISTER(PID, 4);

// extern float tach[12];
// extern float output_pwm[12];
// extern float sensor_value[SENSOR_COUNT];
// extern float air_value[12];
// bool current_type;
// extern const struct device *const usart1;

typedef struct PID_pos
{
    float kp;
    float ki;
    float kd;
    float err_last;
    float err_integral;
    // todo add some member
    // such as: limit etc.
} position_pid;

typedef struct PID_inc
{
    float kp_i;
    float ki_i;
    float kd_i;
    float kp_o;
    float ki_o;
    float kd_o;
    float value_last;
    float err_k;
    float err_k1;
    float err_k2;
    // todo add some member

} increment_pid;

// position_pid sensor_pid[12] = {
//     [0] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [1] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [2] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [3] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [4] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [5] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [6] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [7] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [8] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [9] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [10] = {.kp = 1.0, .ki = 1.0, .kd = 1.0},
//     [11] = {.kp = 1.0, .ki = 1.0, .kd = 1.0}};

increment_pid pos_incr_pid[12] = {
    [0] = {.kp_i = 0.006, .ki_i = 0.0, .kd_i = 0.01,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [1] = {.kp_i = 0.008, .ki_i = 0.00, .kd_i = 0.001,.kp_o = 0.006, .ki_o = 0.00, .kd_o = 0.00},
    [2] = {.kp_i = 0.006, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [3] = {.kp_i = 0.012, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [4] = {.kp_i = 0.012, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [5] = {.kp_i = 0.012, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [6] = {.kp_i = 0.007, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [7] = {.kp_i = 0.006, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [8] = {.kp_i = 0.006, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [9] = {.kp_i = 0.005, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [10] = {.kp_i = 0.005, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000},
    [11] = {.kp_i = 0.005, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.005, .ki_o = 0.0, .kd_o = 0.000}};

// increment_pid tach_pid[12] = {
//     [0] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [1] = {.kp_i = 0.05, .ki_i = 0.00, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [2] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [3] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [4] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [5] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [6] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [7] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [8] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [9] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [10] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000},
//     [11] = {.kp_i = 0.03, .ki_i = 0.0, .kd_i = 0.000,.kp_o = 0.03, .ki_o = 0.0, .kd_o = 0.000}};

static float limit_pid(float value)
{
    value = value > 100 ? 100 : value;
    value = value < -100 ? -100 : value;

    if (value>-20 && value <20){
        value = 0;
    }
    return value;
}

// static float position_sensor_pid(float present, float target, position_pid sensor_pid)
// {
//     float pos_err = target - present;
//     float position_output = 0;

//     sensor_pid.err_integral += pos_err;

//     position_output = sensor_pid.kp * pos_err + sensor_pid.ki * sensor_pid.err_integral + sensor_pid.kd * (pos_err - sensor_pid.err_last);

//     sensor_pid.err_last = pos_err;

//     // is add limit_pid ?
//     return position_output;
// }

static float increment_pos_pid(float present, float target, increment_pid sensor_pid)
{
    float pos_err = target - present;
    float kp, ki , kd;

    if(pos_err < 0){
        kp = sensor_pid.kp_i;
        ki = sensor_pid.ki_i;
        kd = sensor_pid.kd_i;
    }else{
        kp = sensor_pid.kp_o;
        ki = sensor_pid.ki_o;
        kd = sensor_pid.kd_o;
    }
    float increment_output = 0;
    float out_value = 0;
    
    if(present< -60000 || present > 20000){
        out_value = 0 ;
    } else{
        increment_output = kp * (pos_err - sensor_pid.err_k1) + ki * pos_err + kd * (pos_err - 2 * sensor_pid.err_k1 + sensor_pid.err_k2);
        out_value = increment_output + sensor_pid.value_last;
        sensor_pid.err_k1 = pos_err;
        sensor_pid.err_k2 = sensor_pid.err_k1;
        sensor_pid.value_last = increment_output;
    }

    return (out_value);
}

//  Sent utf8 encoded message
// static float increment_tach_pid(float target, float present, increment_pid tach_pid)
// {
//     float pos_err = target - present;
//     float increment_output = 0;
//     float out_value = 0;
//     float kp,ki,kd;
//     if(pos_err < 0){
//         kp = tach_pid.kp_i;
//         ki = tach_pid.ki_i;
//         kd = tach_pid.kd_i;
//     }else{
//         kp = tach_pid.kp_o;
//         ki = tach_pid.ki_o;
//         kd = tach_pid.kd_o;
//     }
//     increment_output = kp * (pos_err - tach_pid.err_k1) + ki * pos_err + kd * (pos_err - 2 * tach_pid.err_k1 + tach_pid.err_k2);

//     out_value = increment_output + tach_pid.value_last;

//     tach_pid.err_k1 = pos_err;
//     tach_pid.err_k2 = tach_pid.err_k1;
//     tach_pid.value_last = increment_output;

//     return (out_value);
// }
/*
 * @brief: pid计算函数，内部实现位置式和增量式pid
 * @param: 当前气压值、串口接收的目标气压值、通道号
 * @return： 计算完输出的pwm值
 */
float pid_calculate_output(float present, float target, int device_id)
{

    // return increment_tach_pid(tach[channel] * 20,
    //                           position_sensor_pid(present, target, sensor_pid[channel]),
    //                           tach_pid[channel]);
    return limit_pid(increment_pos_pid(present, target, pos_incr_pid[device_id - 1]));

}
int pid_init(void)
{
    pid_config_t pid_config;
    int ret = flash_rw_pid_get(&pid_config);
    if (ret < 0) {
        printk("Failed to get PID config from flash\n");
        return ret;
    }
    
}
// static void pid_handle(void *arug0, void *arug1, void *arug2)
// {
//     int i;


//     while(1)
//     {
//         if (current_type) //如果接收的上一包数据是气压数据
//         {   
//             for (i = 0; i < 12; i++)
//             {
//                 output_pwm[i] = limit_pid(pid_calculate(sensor_value[i], air_value[i], i));
//                 // k_msleep(10);
                
//             }         
//         }
      

//         for (i = 0; i < 12; i++)
//         {
//             valve_pwm_output((enum pwm_port)i, output_pwm[i]);
//         }

//         k_msleep(10);
//     }
// }