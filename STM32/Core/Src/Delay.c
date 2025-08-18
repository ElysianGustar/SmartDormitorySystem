#include "delay.h"
#include "stm32f1xx_hal.h"

//延时函数：毫秒级别
void Delay_ms(uint32_t nms)
{
    // 获取系统时钟频率（这里固定为72MHz），并计算SysTick的时钟频率（这里假设SysTick时钟源是系统时钟的1/8）
    uint32_t sysTickClockFreq = 72000000 / 8;
    // SysTick每计数一次的时间（单位：秒），这里计算出每1ms对应的计数次数
    uint32_t countPerMs = sysTickClockFreq / 1000;

    for (uint32_t i = 0; i < nms; i++)
    {
        // 设置SysTick的重装载值
        SysTick->LOAD = countPerMs;
        SysTick->VAL = 0;
        SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;

        // 等待SysTick计数完成
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0)
        {
            // 可以添加一些空操作或者其他需要在延时期间执行的低功耗操作
        }

        SysTick->CTRL = 0;
        SysTick->VAL = 0;
    }
}

// 使用 HAL 库实现微秒级延时函数
void Delay_us(uint32_t nus)
{
    // 获取系统时钟频率（这里固定为72MHz），并计算 SysTick 时钟频率（假设 SysTick 时钟源为系统时钟除以 8）
    uint32_t sysTickClockFreq = 72000000 / 8;
    // 计算每个微秒对应的 SysTick 计数次数
    uint32_t countPerUs = sysTickClockFreq / 1000000;

    for (uint32_t i = 0; i < nus; i++)
    {
        // 设置 SysTick 重装载值
        SysTick->LOAD = countPerUs;
        SysTick->VAL = 0;
        SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;

        // 等待 SysTick 计数完成
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0)
        {
            // 可添加空操作或其他低功耗操作
        }

        SysTick->CTRL = 0;
        SysTick->VAL = 0;
    }
}
