#pragma once

float pid_calculate_output(float present, float target);
float spring_pid_calculate_output(float present, float target);
int pid_init(void);