#include <math.h>
#include <zephyr/sys/printk.h>
#include "pid.h"
#include "flash_rw.h"

/* ============ 最基本位置式 PID ============ */
#define PID_DT        0.01f     // 采样周期（秒），按你的循环频率改，比如 100Hz → 0.01
#define OUT_MIN      -100.0f    // 输出限幅
#define OUT_MAX       100.0f
#define I_MIN      -50000.0f    // 积分限幅（根据你的量纲调整）
#define I_MAX       50000.0f

typedef struct {
    float kp, ki, kd;
    float e_prev;     // 上一次误差
    float i_term;     // 积分项累计
} pid_pos_t;

static pid_pos_t g_pid, spring_pid = {0};

static inline float clampf(float v, float lo, float hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* 一步位置式 PID：u = Kp*e + Ki*∫e dt + Kd*de/dt */
static float pid_pos_step(pid_pos_t* p, float present, float target)
{
    float e   = target - present;
    float de  = (e - p->e_prev) / PID_DT;
    p->i_term = clampf(p->i_term + e * PID_DT, I_MIN, I_MAX);

    float u = p->kp * e + p->ki * p->i_term + p->kd * de;
    p->e_prev = e;

    return clampf(u, OUT_MIN, OUT_MAX);
}

/* 对外接口：返回限幅后的输出 */
float pid_calculate_output(float present, float target)
{
    return pid_pos_step(&g_pid, present, target);
}

float spring_pid_calculate_output(float present, float target)
{
    return pid_pos_step(&spring_pid, present, target);
}

/* 初始化：从 flash 读取 Kp/Ki/Kd；清零历史量 */
int pid_init(void)
{
    pid_config_t pid_in_config, pid_out_config;
    int ret = flash_rw_pid_get(&pid_in_config, &pid_out_config);
    if (ret < 0) {
        printk("Failed to get PID config from flash\n");
        return ret;
    }

    // 位置式只需要一套增益；这里用“正向”这套（你也可以换成 pid_in_config）
    g_pid.kp = pid_out_config.kp;
    g_pid.ki = pid_out_config.ki;
    g_pid.kd = pid_out_config.kd;

    spring_pid.kp = pid_in_config.kp;
    spring_pid.ki = pid_in_config.ki;
    spring_pid.kd = pid_in_config.kd;

    g_pid.e_prev = 0.0f;
    g_pid.i_term = 0.0f;

    spring_pid.e_prev = 0.0f;
    spring_pid.i_term = 0.0f;

    printk("PID(pos) init: Kp=%.6f Ki=%.6f Kd=%.6f, dt=%.3f s\n",
           g_pid.kp, g_pid.ki, g_pid.kd, PID_DT);
    printk("Spring PID(pos) init: Kp=%.6f Ki=%.6f Kd=%.6f, dt=%.3f s\n",
           spring_pid.kp, spring_pid.ki, spring_pid.kd, PID_DT);

    return 0;
}

/* （可选）提供一个重置函数，外部需要时可调用 */
void pid_reset(void)
{
    g_pid.e_prev = 0.0f;
    g_pid.i_term = 0.0f;
}
