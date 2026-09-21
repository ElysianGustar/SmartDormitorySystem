/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "esp8266.h"
#include "onenet.h"
#include "dht11.h"
#include "adc.h"
#include "HS_F04A.h"
#include "OLED.h"
#include "key.h"
#include <string.h>
#include <stdio.h>
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1 , 0xffff);
    return ch;
}

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define INIT_ESP8266_SUCCESS   0x01
#define INIT_ONENET_SUCCESS    0x02
#define INIT_ALL_SUCCESS       (INIT_ESP8266_SUCCESS | INIT_ONENET_SUCCESS)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// esp8266通信
extern unsigned short esp8266_cnt;
extern unsigned char esp8266_buf[128];

uint8_t Uart2_RxData;
uint8_t temp, humi;
uint8_t init_status = 0;
int smoke_value = 0;
Key_ID key;
uint8_t need_reconnect = 0;
uint32_t lastReconnTick = 0;
uint32_t lastPingTick = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern void ESP8266_IRQHandler(void);

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        if(esp8266_cnt >= sizeof(esp8266_buf))
        {            
            esp8266_cnt = 0;
        }
        esp8266_buf[esp8266_cnt++] = Uart2_RxData;
        HAL_UART_Receive_IT(&huart2,(uint8_t *)&Uart2_RxData, 1);
    }		
}

// 获取烟雾数据
int Get_ADC_Value(ADC_HandleTypeDef *hadc) {
    int adc_value = 0;
    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 1);
    adc_value = HAL_ADC_GetValue(hadc)* 100.0f / 4096;
    return adc_value;
}


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    unsigned short timeCount = 0;
    
    /* 系统初始化 */
    HAL_Init();
    SystemClock_Config();
    
    /* 外设初始化 */
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();  // 添加ADC初始化
   
    /* 风扇初始化 */
    HS_F04A_init();
    
    /* OLED初始化 */
    OLED_Init();
    OLED_Clear();
	
	/* 按键初始化 */
	Key_Init();
    
    /* 串口初始化 */
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&Uart2_RxData, 1);
    printf("System Init...\r\n");
    
    /* ESP8266初始化 */
    printf("ESP8266 Init...\r\n");
    ESP8266_Init();
    printf("ESP8266 Init Success\r\n");
    init_status |= INIT_ESP8266_SUCCESS;
    
    /* OneNET连接 */
    printf("OneNET Connecting...\r\n");
    if(!OneNet_DevLink()) {
        printf("OneNET Connect Success\r\n");
        init_status |= INIT_ONENET_SUCCESS;
        
        /* 订阅物模型属性下发,远程控制链路 */
        OneNET_Subscribe();
    } else {
        printf("OneNET Connect Failed\r\n");
        while(1);
    }
    
    /* 系统初始化状态 */
    if((init_status & INIT_ALL_SUCCESS) == INIT_ALL_SUCCESS) {
        printf("All modules initialized successfully\r\n");
    } else {
        printf("System initialization failed. Status: 0x%02X\r\n", init_status);
        if((init_status & INIT_ESP8266_SUCCESS) == 0) {
            printf("- ESP8266 initialization failed\r\n");
        }
        if((init_status & INIT_ONENET_SUCCESS) == 0) {
            printf("- OneNET connection failed\r\n");
        }
        while(1);
    }
    
    /* 设置PB12为高电平输出 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    
    /* 主循环 */
    lastReconnTick = HAL_GetTick();
    lastPingTick = HAL_GetTick();
    while (1)
    {
        /* 按键控制蜂鸣器 */
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_RESET) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);  // 蜂鸣器响
        } else {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);  // 蜂鸣器不响
        }
		
		if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_RESET) {
            HS_F04A_Ctrl(MOTOR_FORWARD);
        } else {
            HS_F04A_Ctrl(MOTOR_STOP);
        }

        /* 轮询 OneNET 下发数据(命令 / 物模型属性设置) */
        if(ESP8266_WaitRecive() == REV_OK)
        {
            if(ESP8266_LinkLost())
            {
                printf("Link Lost\r\n");
                need_reconnect = 1;
            }
            else
            {
                unsigned char *ipd = (unsigned char *)strstr((char *)esp8266_buf, "IPD,");
                if(ipd != NULL)
                {
                    ipd = (unsigned char *)strchr((char *)ipd, ':');
                    if(ipd != NULL)
                        OneNet_RevPro(ipd + 1);
                    else
                        ESP8266_Clear();
                }
                else
                {
                    ESP8266_Clear();
                }
            }
        }

        /* 每秒周期:采集传感器数据并上报 */
        if(++timeCount >= 100)
        {
            timeCount = 0;
            
            /* 获取传感器数据 */
            if(DHT11_Read_Data(&temp, &humi) == 0) {    // 读取温度、湿度值
                smoke_value = Get_ADC_Value(&hadc1);     // 读取烟雾传感器值
				
				if (smoke_value >= 25) {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);  // 蜂鸣器响
				} else {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);  // 蜂鸣器不响
				}
				
                /* OLED显示数据 */
                OLED_Clear();
                OLED_ShowString(1, 1, "Temp:");
                OLED_ShowNum(1, 7, temp, 2);
                OLED_ShowString(1, 10, "C");
                
                OLED_ShowString(2, 1, "Humi:");
                OLED_ShowNum(2, 7, humi, 2);
                OLED_ShowString(2, 10, "%");
                
                OLED_ShowString(3, 1, "Smoke:");
                OLED_ShowNum(3, 8, smoke_value, 3);
                OLED_ShowString(3, 12, "ppm");
                
                OneNet_SendData();
                printf("temp: %d, humi: %d, smoke: %d\r\n", temp, humi, smoke_value);
                ESP8266_Clear();
            } else {
                printf("DHT11 Read Error\r\n");
            }
        }

        /* 断线自动重连(间隔至少 30s 尝试一次) */
        if(need_reconnect && (HAL_GetTick() - lastReconnTick) >= 30000)
        {
            printf("OneNET Reconnecting...\r\n");
            if(OneNet_Reconnect() == 0)
                need_reconnect = 0;
            lastReconnTick = HAL_GetTick();
        }

        /* MQTT 心跳:每 60s 发送 PINGREQ,防止空闲被服务器断开 */
        if((HAL_GetTick() - lastPingTick) >= 60000)
        {
            OneNet_KeepAlive();
            lastPingTick = HAL_GetTick();
        }

        HAL_Delay(10);
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
