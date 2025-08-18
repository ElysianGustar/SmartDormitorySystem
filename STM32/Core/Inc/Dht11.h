#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

//DHT11_DATA ,IO口设置为输出模式（这里的IO口指的是STM32CubeMX中配置的IO口）
void DHT11_OUT(void);	
												
void DHT11_IN(void);                          //DHT11_Data IO设置为输入模式
void DHT11_Strat(void);				          //主机发出起始信号
uint8_t DHT11_Check(void);                    //DHT11发送响应信号
uint8_t DHT11_Read_Bit(void);                 //读取DHT11一个BIT的数据
uint8_t DHT11_Read_Byte(void);                //读取DHT11一个Byte的数据
uint8_t DHT11_Read_Data(uint8_t* temp , uint8_t* humi);  //读取DHT11湿度和温度的数据
void DHT11_Delay_us(uint32_t udelay);         //DHT11专用的us级延时函数

/* USER CODE BEGIN PFP */
#endif
