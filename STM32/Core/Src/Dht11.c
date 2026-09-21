#include "main.h"
#include "Dht11.h"
#include "delay.h"
#include "usart.h"   // huart2 for ESP8266 RX protection during timing read

/* USER CODE BEGIN PD */
#define DHT11_Pin GPIO_PIN_11
#define DHT11_GPIO_Port GPIOB


#define DHT11_HIGH     HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin,	GPIO_PIN_SET) // set HIGH
#define DHT11_LOW      HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET)// set LOW
 
#define DHT11_IO_IN    HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)// read pin level
 
/* USER CODE END PD */
/**
  * @brief  Configure DATA pin as push-pull output
  * @param  none
  * @retval none
  */
void DHT11_OUT(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct = {0};
 
	GPIO_InitStruct.Pin = GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}
/**
  * @brief  Configure DATA pin as input
  * @param  none
  * @retval none
  */
void DHT11_IN(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct = {0};
 
	GPIO_InitStruct.Pin  = GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief  microsecond delay based on SysTick counter
  * @param  udelay delay in microseconds
  * @retval none
  */
void DHT11_Delay_us(uint32_t udelay)
{
    uint32_t startval,tickn,delays,wait;
    startval = SysTick->VAL;
    tickn = HAL_GetTick();
    delays =udelay * 72;
    if(delays > startval)
    {
        while(HAL_GetTick() == tickn)
        {
        }
        wait = 72000 + startval - delays;
        while(wait < SysTick->VAL)
        {
        }
    }
    else
    {
        wait = startval - delays;
        while(wait < SysTick->VAL && HAL_GetTick() == tickn)
        {
        }
    }
}

/**
  * @brief  DHT11 start signal
  * @param  none
  * @retval none
  */
void DHT11_Strat(void)
{
	DHT11_OUT();   // configure as output
	DHT11_LOW;     // pull low
	HAL_Delay(20); // keep low >= 18ms
	DHT11_HIGH;    // pull high, wait for response
	DHT11_Delay_us(30);   
}

/**
  * @brief  check DHT11 response signal
  * @param  none
  * @retval 0-ok 1-fail
  */
uint8_t DHT11_Check(void)
{
	uint8_t retry = 0 ;
	DHT11_IN();
	// wait for response low level (40us ~ 80us)
	while(DHT11_IO_IN && retry <100)
	{
		retry++;
		DHT11_Delay_us(1);//1us
	}
	if(retry>=100) // delay exceeds 80us
	{return  1;}
	else retry =  0 ;
		
	while(!DHT11_IO_IN && retry<100)// then high level
	{
		retry++;
		DHT11_Delay_us(1);//1us
	}
		
	if(retry>=100)
	{return 1;}
	return 0 ;
}

/**
  * @brief  read one bit from DHT11
  * @param  none
  * @retval 1-bit high 0-bit low
  */
uint8_t DHT11_Read_Bit(void)
{
	uint8_t retry = 0 ;
	while(DHT11_IO_IN && retry <100)// wait low level
	{
		retry++;
		DHT11_Delay_us(1);
	}
	retry = 0 ;
	while(!DHT11_IO_IN && retry<100)
	{
		retry++;
		DHT11_Delay_us(1);
	}
 
	DHT11_Delay_us(40);              // sample after 40us
	if(DHT11_IO_IN) return 1;  // still high -> bit 1
	else 
	return 0 ;
}

/**
  * @brief  read one byte from DHT11
  * @param  none
  * @retval dat the byte just read
  */
uint8_t DHT11_Read_Byte(void)
{
	uint8_t i , dat ;
	dat = 0 ;
	for(i=0; i<8; i++)
	{
		dat <<= 1; // shift to store bits
		dat |= DHT11_Read_Bit();
	}
	return dat ; 
}

/**
  * @brief  read temperature and humidity
  * @param  temp temperature value, humi humidity value
  * @retval 0-read ok 1-read fail
  */
uint8_t DHT11_Read_Data(uint8_t* temp , uint8_t* humi)
{
	uint8_t buf[5];        // store 5 bytes
    uint8_t i;
	uint8_t err = 1;       // fail by default
	DHT11_Strat();         // start signal
	
	/* disable interrupts to protect us-level timing, prevent USART2 (ESP8266)
	 * RX interrupt from corrupting the timing read */
	__disable_irq();
	
	if(DHT11_Check() == 0) // response ok
    {
		for(i=0; i<5; i++)
		{
			buf[i] = DHT11_Read_Byte();
		}
		if(buf[0]+buf[1]+buf[2]+buf[3] == buf[4]) // checksum ok
		{
		    *humi = buf[0]; // humidity
			*temp = buf[2]; // temperature
			err = 0;
		}
	}
	
	/* drop any byte buffered by USART2 during the blocking read and clear the
	 * overrun flag, otherwise ORE would break the ESP8266 receive state */
	if(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE))
		(void)huart2.Instance->DR;
	__HAL_UART_CLEAR_OREFLAG(&huart2);
	__enable_irq();
	
	return err;
}