#include "esp8266.h"

//协议文件
#include "onenet.h"
#include "mqttkit.h"

//硬件驱动
//#include "usart.h"
//#include "delay.h"
//#include "led.h"
#include "main.h"
#include "dht11.h"
//C库
#include <string.h>
#include <stdio.h>


#define PROID        "xUHsdh4wh3"

#define AUTH_INFO    "version=2018-10-31&res=products%2FxUHsdh4wh3%2Fdevices%2Ftest&et=2058528898&method=md5&sign=yj4TmkDq7AGJcRp%2FZNZLFw%3D%3D"

#define DEVID        "test"


extern unsigned char esp8266_buf[128];
extern uint8_t Data[5];
extern uint8_t temp, humi;
extern int smoke_value;

//==========================================================
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================


_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

	unsigned char *dataPtr;
	
	_Bool status = 1;
	
	printf("OneNet_DevLink\r\n"
							"PROID: %s,	AUIF: %s,	DEVID:%s\r\n"
                        , PROID, AUTH_INFO, DEVID);
	
	if(MQTT_PacketConnect(PROID, AUTH_INFO, DEVID, 256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//上传平台
		
		dataPtr = ESP8266_GetIPD(250);									//等待平台响应
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:printf("Tips: Connect Success\r\n");status = 0;break;
					
					case 1:printf("WARN: Connect Failed: Protocol Error\r\n");break;
					case 2:printf("WARN: Connect Failed: Invalid ClientID\r\n");break;
					case 3:printf("WARN: Connect Failed: Server Error\r\n");break;
					case 4:printf("WARN: Connect Failed: Username or Password Error\r\n");break;
					case 5:printf("WARN: Connect Failed: Invalid Connection\r\n");break;
					
					default:printf("ERR: Connect Failed: Unknown Error\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//删包
	}
	else
		printf("WARN:	MQTT_PacketConnect Failed\r\n");
	
	HAL_Delay(500);
	return status;
	
}

unsigned char OneNet_FillBuf(char *buf)
{
	
	char text[55];
 
	
	strcpy(buf,"{\"id\":\"123\",\"params\":{");

	
	memset(text, 0, sizeof(text));
	sprintf(text,"\"temp\":{\"value\":%d},", temp);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text,"\"humi\":{\"value\":%d},", humi);
	strcat(buf, text);

	memset(text, 0, sizeof(text));
	sprintf(text,"\"MQ2\":{\"value\":%d}", smoke_value);
	strcat(buf, text);
	
	strcat(buf,"}}");
	
	return strlen(buf);

}

//unsigned char OneNet_FillBuf_Temp(char *buf)
//{
//    char text[48];

//    memset(text, 0, sizeof(text));

//    strcpy(buf, "{\"id\":\"123\",\"params\":{");

//    memset(text, 0, sizeof(text));
//    sprintf(text, "\"humi\":{\"value\":%d}", Data[0]);
//    strcat(buf, text);

//    strcat(buf, "}}");

//    return strlen(buf);
//}

//unsigned char OneNet_FillBuf_Light(char *buf)
//{
//    char text[48];

//    memset(text, 0, sizeof(text));

//    strcpy(buf, "{\"id\":\"123\",\"params\":{");

//    memset(text, 0, sizeof(text));
//    sprintf(text, "\"humi\":{\"value\":%d}", Data[0]);
//    strcat(buf, text);

//    strcat(buf, "}}");

//    return strlen(buf);
//}


//unsigned char OneNet_FillBufll_Buf_MQ2(char *buf)
//{
//    char text[48];

//    memset(text, 0, sizeof(text));

//    strcpy(buf, "{\"id\":\"123\",\"params\":{");

//    memset(text, 0, sizeof(text));
//    sprintf(text, "\"humi\":{\"value\":%d}", Data[0]);
//    strcat(buf, text);

//    strcat(buf, "}}");

//    return strlen(buf);
//}


//==========================================================
//	函数名称：	OneNet_SendData
//
//	函数功能：	上传数据到平台
//
//	入口参数：	type：发送数据的格式
//
//	返回参数：	无
//
//	说明：		
//==========================================================
 
//void OneNet_SendData_Humi(void)
//{

//    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};                                                //协议包

//    char buf[256];

//    short body_len = 0, i = 0;

//    printf("Tips:  OneNet_SendData-Humi-MQTT\r\n");

//    memset(buf, 0, sizeof(buf));

//    body_len = OneNet_FillBuf_Humi(buf);                                                                 //获取当前需要发送的数据流的总长度

//    if (body_len)
//    {
//        if (MQTT_PacketSaveData(DEVID, body_len, NULL, 5, &mqttPacket) == 0)                         //封包
//        {
//            for (; i < body_len; i++)
//                mqttPacket._data[mqttPacket._len++] = buf[i];

//            ESP8266_SendData(mqttPacket._data, mqttPacket._len);                                     //上传数据到平台
//            printf("Send %d Bytes\r\n", mqttPacket._len);

//            MQTT_DeleteBuffer(&mqttPacket);                                                         //删包
//        }
//        else
//            printf("WARN:  EDP_NewBuffer Failed\r\n");
//    }

//}
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//协议包
	
	char buf[128];
	
	short body_len = 0, i = 0;
	
	printf("Tips:	OneNet_SendData-MQTT\r\n");
	
	memset(buf, 0, sizeof(buf));
	
	body_len = OneNet_FillBuf(buf);																	//获取当前需要发送的数据流的总长度
	printf("%s\r\n",buf);
	if(body_len)
	{
		if(MQTT_PacketSaveData(DEVID, body_len, NULL, 5, &mqttPacket) == 0)							//封包
		{
			for(; i < body_len; i++)
			{
		    		mqttPacket._data[mqttPacket._len++] = buf[i];
			}
				
//			printf("%s\r\n",mqttPacket._data);
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//上传数据到平台
			printf("Send %d Bytes\r\n", mqttPacket._len);
			
			MQTT_DeleteBuffer(&mqttPacket);															//删包
		}
		else
			printf("WARN:	EDP_NewBuffer Failed\r\n");
	}
	
}
//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};								//协议包
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short req_len = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_CMD:															//命令下发
			
			result = MQTT_UnPacketCmd(cmd, &cmdid_topic, &req_payload, &req_len);	//解出topic和消息体
			if(result == 0)
			{
				printf("cmdid: %s, req: %s, req_len: %d\r\n", cmdid_topic, req_payload, req_len);
				
				if(MQTT_PacketCmdResp(cmdid_topic, req_payload, &mqttPacket) == 0)	//命令回复组包
				{
					printf("Tips:	Send CmdResp\r\n");
					
					ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//回复命令
					MQTT_DeleteBuffer(&mqttPacket);									//删包
				}
			}
		
		break;
			
		case MQTT_PKT_PUBACK:														//发送Publish消息，平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				printf("Tips:	MQTT Publish Send OK\r\n");
			
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//清空缓存
	
	if(result == -1)
		return;
	
	dataPtr = strchr(req_payload, ':');					//搜索':'

	if(dataPtr != NULL && result != -1)					//如果找到了
	{
		dataPtr++;
		
		while(*dataPtr >= '0' && *dataPtr <= '9')		//判断是否是下发的命令控制数据
		{
			numBuf[num++] = *dataPtr++;
		}
		numBuf[num] = 0;
		
		num = atoi((const char *)numBuf);				//转为数值形式
		
		if(strstr((char *)req_payload, "redled"))		//搜索"redled"
		{
			if(num == 1)								//控制数据如果为1，代表开
			{
				//Led5_Set(LED_ON);
                printf("Led5_Set(LED_ON");
			}
			else if(num == 0)							//控制数据如果为0，代表关
			{
				//Led5_Set(LED_OFF);
                printf("Led5_Set(LED_OFF");
			}
		}
														//下同
		else if(strstr((char *)req_payload, "greenled"))
		{
			if(num == 1)
			{
				//Led4_Set(LED_ON);
                printf("Led4_Set(LED_ON");
			}
			else if(num == 0)
			{
				//Led4_Set(LED_OFF);
                printf("Led4_Set(LED_OFF");
			}
		}
		else if(strstr((char *)req_payload, "yellowled"))
		{
			if(num == 1)
			{
				//Led3_Set(LED_ON);
                printf("Led3_Set(LED_ON");
			}
			else if(num == 0)
			{
				//Led2_Set(LED_OFF);
                printf("Led2_Set(LED_OFF");
			}
		}
		else if(strstr((char *)req_payload, "blueled"))
		{
			if(num == 1)
			{
				//Led2_Set(LED_ON);
                printf("Led2_Set(LED_ON");
			}
			else if(num == 0)
			{
				//Led2_Set(LED_OFF);
                printf("Led2_Set(LED_OFF");
			}
		}
	}

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}

void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	//UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}


void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	char topic_buf[56];
	const char *topic = topic_buf;
	
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set", PROID, DEVID);
	
	printf("Subscribe Topic: %s\r\n", topic_buf);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}
}
