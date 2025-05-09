#pragma once

float pid_calculate_output(float present, float target, int device_id);
int pid_init(void);