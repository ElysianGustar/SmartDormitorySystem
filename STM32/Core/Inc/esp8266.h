#ifndef _ESP8266_H_
#define _ESP8266_H_





#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志
#include "main.h"

void ESP8266_Init(void);

void ESP8266_Clear(void);

void ESP8266_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP8266_GetIPD(unsigned short timeOut);

void ESP8266_IRQHandler(void);

uint8_t ESP8266_LinkLost(void);

void ESP8266_ResetLinkLost(void);

uint8_t ESP8266_Reconnect(void);


#endif
