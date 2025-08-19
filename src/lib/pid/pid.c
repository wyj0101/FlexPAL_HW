#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

#include "pid.h"
#include "flash_rw.h"

// LOG_MODULE_REGISTER(PID, 4);
// ── 方向切换缓冲参数 ───────────────────────────────
// #define PWM_MIN_ON               60.0f   // 你的硬件起转阈值：跨过死区才允许动作
// #define SWITCH_SETTLE_TICKS       5      // 缓冲持续的控制周期数（先 5，再按手感调）
// #define SWITCH_MICRO_STEP         2.0f   // 每个周期向目标迈的最大步长（1~3常用）
// #define SETTLE_POS_TARGET        65.0f   // 正侧缓冲目标（+65）
// #define SETTLE_NEG_TARGET       -65.0f   // 负侧缓冲目标（-65）

// static int   s_settle_ticks  = 0;        // 缓冲剩余拍数
// static float s_settle_target = 0.0f;     // 本次缓冲目标（+65 或 -65）
// static int   s_prev_sign     = 0;        // 上一拍符号：-1/0/+1

// 放在函数顶部附近
static inline float ki_scale_for_target(float target_abs)
{
    const float A = 20000.0f;   // 20 kPa
    const float B = 30000.0f;   // 30 kPa
    const float S_AT_A = 1.0f;
    const float S_AT_B = 0.25f; // 深区把 Ki 降到 25%
    if (target_abs <= A) return S_AT_A;
    if (target_abs >= B) return S_AT_B;
    float r = (target_abs - A) / (B - A);         // 0..1
    return S_AT_A + r * (S_AT_B - S_AT_A);        // 线性插值
}

typedef struct PID_pos
{
    float kp;
    float ki;
    float kd;
    float err_last;
    float err_integral;
} position_pid;

typedef struct PID_inc
{
    float kp_i;
    float ki_i;
    float kd_i;
    float kp_o;
    float ki_o;
    float kd_o;
    float value_last;   // 累计输出的“上一次值”（用于增量式求和）
    float err_k;
    float err_k1;      // e[k-1]
    float err_k2;      // e[k-2]
} increment_pid;

// ==== 你保留的全局 ====
increment_pid pos_incr_pid = {0};

static float limit_pid(float value)
{
    value = value > 100 ? 100 : value;
    value = value < -100 ? -100 : value;

    if (value > -50 && value < 50){
        value = 40;
    }
    return value;
}

// ================== 修复后的关键函数 ==================
// 1) 传指针：increment_pid* sensor_pid
// 2) 历史误差更新顺序修正：err_k2 <- old err_k1，再 err_k1 <- pos_err
// 3) 累计输出记忆修正：value_last 保存的是 out_value（而不是 increment 本身）
static float increment_pos_pid(float present, float target, increment_pid *sensor_pid)
{
    float pos_err = target - present;
    float kp, ki , kd;

    if (pos_err < 0){
        kp = sensor_pid->kp_i;
        ki = sensor_pid->ki_i;
        kd = sensor_pid->kd_i;
    } else {
        kp = sensor_pid->kp_o;
        ki = sensor_pid->ki_o;
        kd = sensor_pid->kd_o;
    }

    float increment_output = 0.0f;
    float out_value = 0.0f;

    // 测量值防呆：保持你原来的范围判断
    if (present < -60000 || present > 20000) {
        out_value = sensor_pid->value_last;  // 异常时保持，不骤变
    } else {
        float ki_eff = ki * ki_scale_for_target(fabsf(target));
        // Δu = kp*(e[k]-e[k-1]) + ki*e[k] + kd*(e[k]-2e[k-1]+e[k-2])
        increment_output = kp * (pos_err - sensor_pid->err_k1)
                         + ki_eff * (pos_err)
                         + kd * (pos_err - 2 * sensor_pid->err_k1 + sensor_pid->err_k2);

        // 累计：u[k] = u[k-1] + Δu
        out_value = increment_output + sensor_pid->value_last;

        // ---- 历史项更新顺序（重要）----
        sensor_pid->err_k2 = sensor_pid->err_k1;  // 先把旧的 e[k-1] 赋给 e[k-2]
        sensor_pid->err_k1 = pos_err;             // 再更新 e[k-1] = e[k]
        // --------------------------------

        // 累计输出记忆保存（重要）：保存累计后的 out_value
        sensor_pid->value_last = out_value;
    }

    return out_value;
}


/*
 * @brief: pid计算函数，内部实现位置式和增量式pid
 * @param: 当前气压值、串口接收的目标气压值
 * @return： 计算完输出的pwm值
 */
float pid_calculate_output(float present, float target)
{
    // 注意：这里传指针 &pos_incr_pid
    return limit_pid(increment_pos_pid(present, target, &pos_incr_pid));
}


// #define TGT_CHANGE_DB   200.0f   // 判定阈值：目标变动超过200Pa算“目标变了”

// float pid_calculate_output(float present, float target)
// {
//     static int   s_has_last_target = 0;
//     static float s_last_target     = 0.0f;

//     if (!s_has_last_target) {
//         s_has_last_target = 1;
//         s_last_target = target;
//     } else {
//         float dtgt = target - s_last_target;
//         if (dtgt > TGT_CHANGE_DB || dtgt < -TGT_CHANGE_DB) {
//             // ★ 清掉积分累积（关键）
//             pos_incr_pid.value_last *= 0.5f;
//         }
//         s_last_target = target;
//     }

//     float u = increment_pos_pid(present, target, &pos_incr_pid);
//     u = limit_pid(u);
//     pos_incr_pid.value_last = u;   // 更新累计
//     return u;
// }

int pid_init(void)
{
    pid_config_t pid_in_config, pid_out_config;
    int ret = flash_rw_pid_get(&pid_in_config, &pid_out_config);
    if (ret < 0) {
        printk("Failed to get PID config from flash\n");
        return ret;
    }

    // 误差<0 使用的参数
    pos_incr_pid.kp_i = pid_in_config.kp;
    pos_incr_pid.ki_i = pid_in_config.ki;
    pos_incr_pid.kd_i = pid_in_config.kd;

    // 误差>=0 使用的参数
    pos_incr_pid.kp_o = pid_out_config.kp;
    pos_incr_pid.ki_o = pid_out_config.ki;
    pos_incr_pid.kd_o = pid_out_config.kd;

    // 可选：上电清零一次
    pos_incr_pid.value_last = 0.0f;
    pos_incr_pid.err_k1 = 0.0f;
    pos_incr_pid.err_k2 = 0.0f;

    return 0; // 补返回值
}
