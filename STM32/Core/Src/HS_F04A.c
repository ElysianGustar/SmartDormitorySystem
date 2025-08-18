/*
 *  HS-F04A.c
 *  (C) 2025  Guo Xin
 */
#include "main.h"
#include "HS_F04A.h"

#define GPIO_Port GPIOB
#define	IN_A	GPIO_PIN_12
#define	IN_B	GPIO_PIN_13

/*
 * 初始化HS-F04A
 * 无返回值
 */
void HS_F04A_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitStruct.Pin = IN_A | IN_B;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIO_Port, &GPIO_InitStruct);
	
	// 初始化时设置为停止状态
	HS_F04A_Ctrl(MOTOR_STOP);
}

/*
 * 控制HS-F04A
 * state: 电机状态（停止/正转/反转）
 * 无返回值
 */
void HS_F04A_Ctrl(Motor_State state)
{
	switch(state)
	{
		case MOTOR_STOP:
			// 停止：两个引脚设置为相同电平（这里选择低电平）
			HAL_GPIO_WritePin(GPIO_Port, IN_A, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIO_Port, IN_B, GPIO_PIN_RESET);
			break;
			
		case MOTOR_FORWARD:
			// 正转：IN_A=1, IN_B=0
			HAL_GPIO_WritePin(GPIO_Port, IN_A, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIO_Port, IN_B, GPIO_PIN_RESET);
			break;
			
		case MOTOR_REVERSE:
			// 反转：IN_A=0, IN_B=1
			HAL_GPIO_WritePin(GPIO_Port, IN_A, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIO_Port, IN_B, GPIO_PIN_SET);
			break;
			
		default:
			// 默认停止
			HAL_GPIO_WritePin(GPIO_Port, IN_A, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIO_Port, IN_B, GPIO_PIN_RESET);
			break;
	}
}
