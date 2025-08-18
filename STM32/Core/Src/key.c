/*
 *  key.c
 *  (C) 2025  Guo Xin
 */
#include "key.h"

void Key_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    KEY_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = KEY_PINS;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;      // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;         // 上拉电阻（按键接GND）
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY_PORT, &GPIO_InitStruct);
}

// 支持长按/短按
Key_ID Key_Scan(uint32_t long_press_threshold) {
    static uint32_t press_tick[2] = {0};
    static uint8_t key_up[2] = {1,1};
    
    for(uint8_t i=0; i<2; i++) {
        GPIO_PinState pin_state = HAL_GPIO_ReadPin(KEY_PORT, GPIO_PIN_14 << i);
        
        if(pin_state == GPIO_PIN_RESET) { 
            HAL_Delay(10); // 10ms消抖
            if(HAL_GPIO_ReadPin(KEY_PORT, GPIO_PIN_14 << i) == GPIO_PIN_RESET) {
                if(key_up[i]) { // 首次按下
                    key_up[i] = 0;
                    press_tick[i] = HAL_GetTick();
                } else { // 持续按下
                    if(HAL_GetTick() - press_tick[i] > long_press_threshold) {
                        return (Key_ID)(i + 1 + 2); // 长按编号3-4
                    }
                }
            }
        } else {
            if(!key_up[i]) { // 松开检测
                key_up[i] = 1;
                return (Key_ID)(i + 1); // 短按编号1-2
            }
        }
    }
    return KEY_NONE;
}
