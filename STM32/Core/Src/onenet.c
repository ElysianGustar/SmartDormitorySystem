#include "esp8266.h"

//协议文件
#include "onenet.h"
#include "mqttkit.h"

//硬件驱动
#include "main.h"
#include "net_config.h"
#include "dht11.h"
//C库
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


extern unsigned char esp8266_buf[128];
extern uint8_t temp, humi;
extern int smoke_value;
extern uint8_t buzzer_enable;

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
				
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//上传数据到平台
			printf("Send %d Bytes\r\n", mqttPacket._len);
			
			MQTT_DeleteBuffer(&mqttPacket);															//删包
		}
		else
			printf("WARN:	EDP_NewBuffer Failed\r\n");
	}
	
}
//==========================================================
//	函数名称：	OneNet_ParseJsonProp
//
//	函数功能：	从 JSON 属性下发报文中解析指定键的值
//
//	入口参数：	payload：报文内容
//				key：属性标识(如 "led")
//
//	返回参数：	-1-未找到	其他-解析出的数值(true=1, false=0)
//
//	说明：		
//==========================================================
static int OneNet_ParseJsonProp(const char *payload, const char *key)
{
	char buf[32];
	char *p = NULL;

	snprintf(buf, sizeof(buf), "\"%s\"", key);
	p = strstr((char *)payload, buf);
	if(p == NULL)
		return -1;

	p += strlen(buf);
	p = strchr(p, ':');
	if(p == NULL)
		return -1;

	p++;
	while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;

	if(strncmp(p, "true", 4) == 0)
		return 1;
	if(strncmp(p, "false", 5) == 0)
		return 0;

	return atoi(p);
}

//==========================================================
//	函数名称：	OneNet_ExecCommand
//
//	函数功能：	执行平台下发的控制命令
//
//	入口参数：	payload：命令内容
//
//	返回参数：	无
//
//	说明：		当前实现:led 属性下发直接控制蜂鸣器(PB10)
//==========================================================
static void OneNet_ExecCommand(const char *payload)
{
	int led = -1;

	if(payload == NULL || payload[0] == 0)
		return;

	/* 物模型属性下发,格式为 {"params":{...,"led":1}} 或 {"led":1} */
	led = OneNet_ParseJsonProp(payload, "led");
	if(led >= 0)
	{
		buzzer_enable = (uint8_t)(led ? 1 : 0);		//与本地按键共用同一个蜂鸣器开关,避免互相覆盖
		printf("Remote Set led = %d\r\n", led);
	}
}

//==========================================================
//	函数名称：	OneNet_ReplyPropertySet
//
//	函数功能：	回复物模型属性设置结果
//
//	入口参数：	payload：收到的属性设置报文
//
//	返回参数：	无
//
//	说明：		
//==========================================================
static void OneNet_ReplyPropertySet(const char *topic, const char *payload)
{
	char reply_topic[64];
	char reply[64];
	char msgid[16] = "0";
	char *p = NULL;
	int i = 0;
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};

	/* 提取上报报文中的 id,回复时原样带回 */
	p = strstr((char *)payload, "\"id\"");
	if(p != NULL)
	{
		p = strchr(p, ':');
		if(p != NULL)
		{
			p++;
			while(*p == ' ' || *p == '\t')
				p++;
			while(p[i] >= '0' && p[i] <= '9' && i < 15)
			{
				msgid[i] = p[i];
				i++;
			}
			msgid[i] = 0;
		}
	}

	if(strstr(topic, "/desired/") != NULL)
		snprintf(reply_topic, sizeof(reply_topic), "$sys/%s/%s/thing/property/desired/set_reply", PROID, DEVID);
	else
		snprintf(reply_topic, sizeof(reply_topic), "$sys/%s/%s/thing/property/set_reply", PROID, DEVID);

	snprintf(reply, sizeof(reply), "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\",\"data\":{}}", msgid);

	printf("Property Set Reply: %s\r\n", reply);

	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, reply_topic, reply, strlen(reply), MQTT_QOS_LEVEL0, 0, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);
		MQTT_DeleteBuffer(&mqttPacket);
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
	unsigned char type = MQTT_UnPacketRecv(cmd);

	switch(type)
	{
		case MQTT_PKT_CMD:									//旧版 $creq 命令下发
		{
			char *cmdid_topic = NULL;
			char *req_payload = NULL;
			unsigned short req_len = 0;
			MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};

			if(MQTT_UnPacketCmd(cmd, &cmdid_topic, &req_payload, &req_len) == 0)
			{
				printf("cmdid: %s, req: %s, req_len: %d\r\n", cmdid_topic, req_payload, req_len);

				OneNet_ExecCommand(req_payload);

				if(MQTT_PacketCmdResp(cmdid_topic, req_payload, &mqttPacket) == 0)
				{
					printf("Tips:	Send CmdResp\r\n");
					ESP8266_SendData(mqttPacket._data, mqttPacket._len);
					MQTT_DeleteBuffer(&mqttPacket);
				}
			}

			MQTT_FreeBuffer(cmdid_topic);
			MQTT_FreeBuffer(req_payload);
		}
		break;

		case MQTT_PKT_PUBLISH:								//物模型属性下发 thing/property/set
		{
			char *topic = NULL;
			char *req_payload = NULL;
			unsigned short topic_len = 0, payload_len = 0;
			uint8 qos = 0;
			uint16 pkt_id = 0;

			if(MQTT_UnPacketPublish(cmd, &topic, &topic_len, &req_payload, &payload_len, &qos, &pkt_id) == 0)
			{
				if(strstr(topic, "property/set") != NULL)
				{
					printf("PropertySet: %s\r\n", req_payload);

					OneNet_ExecCommand(req_payload);
					OneNet_ReplyPropertySet(topic, req_payload);
				}

				if(qos == MQTT_QOS_LEVEL1)
				{
					MQTT_PACKET_STRUCTURE ackPacket = {NULL, 0, 0, 0};
					if(MQTT_PacketPublishAck(pkt_id, &ackPacket) == 0)
					{
						ESP8266_SendData(ackPacket._data, ackPacket._len);
						MQTT_DeleteBuffer(&ackPacket);
					}
				}
			}

			MQTT_FreeBuffer(topic);
			MQTT_FreeBuffer(req_payload);
		}
		break;

		case MQTT_PKT_PUBACK:								//发送Publish消息，平台回复的Ack
		{
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				printf("Tips:	MQTT Publish Send OK\r\n");
		}
		break;

		default:
		break;
	}

	ESP8266_Clear();									//清空缓存
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
	
	char topic_buf_1[64];
	char topic_buf_2[64];
	const char *topics[2];
	
	//物模型属性下发 topic 及期望属性下发 topic
	snprintf(topic_buf_1, sizeof(topic_buf_1), "$sys/%s/%s/thing/property/set", PROID, DEVID);
	snprintf(topic_buf_2, sizeof(topic_buf_2), "$sys/%s/%s/thing/property/desired/set", PROID, DEVID);
	topics[0] = topic_buf_1;
	topics[1] = topic_buf_2;
	
	printf("Subscribe Topics: %s, %s\r\n", topic_buf_1, topic_buf_2);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, topics, 2, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}
}

//==========================================================
//	函数名称：	OneNet_KeepAlive
//
//	函数功能：	发送 MQTT 心跳包,防止空闲被服务器断开
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_KeepAlive(void)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};

	if(MQTT_PacketPing(&mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);
		printf("Tips:	MQTT PINGREQ Sent\r\n");
		MQTT_DeleteBuffer(&mqttPacket);
	}
}

//==========================================================
//	函数名称：	OneNet_Reconnect
//
//	函数功能：	断线后自动重连(WiFi/TCP + MQTT + 重新订阅)
//
//	入口参数：	无
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
uint8_t OneNet_Reconnect(void)
{
	if(ESP8266_Reconnect() != 0)								//先恢复 TCP 连接
		return 1;

	if(OneNet_DevLink() != 0)									//重新 MQTT 接入
		return 1;

	OneNET_Subscribe();											//重新订阅属性下发
	ESP8266_ResetLinkLost();									//清除断线标志

	printf("OneNET Reconnect Success\r\n");
	return 0;
}
