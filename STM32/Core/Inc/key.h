#ifndef __KEY_H
#define __KEY_H

#include "main.h"

#define KEY_PORT        GPIOB
#define KEY_PINS        (GPIO_PIN_14 | GPIO_PIN_15)
#define KEY_CLK_ENABLE()	__HAL_RCC_GPIOB_CLK_ENABLE()

// 按键编号枚举
typedef enum {
    KEY_NONE = 0,
    KEY_1, KEY_2,                    // 短按状态
    KEY_LONG_1, KEY_LONG_2           // 长按状态
} Key_ID;
void Key_Init(void);
Key_ID Key_Scan(uint32_t long_press_threshold);

#endif
