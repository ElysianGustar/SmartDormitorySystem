#ifndef _HS_F04A_H
#define _HS_F04A_H

#include "main.h"  // 为了使用 GPIO_PinState 类型

// 定义电机状态枚举
typedef enum {
    MOTOR_STOP = 0,    // 停止
    MOTOR_FORWARD,     // 正转
    MOTOR_REVERSE      // 反转
} Motor_State;

void HS_F04A_init(void);
void HS_F04A_Ctrl(Motor_State state);

#endif


