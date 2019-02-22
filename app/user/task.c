/*
 * task.c
 *
 *  Created on: 2019年1月31日
 *      Author: BBA
 */

#include "task.h"
extern struct BBA_FlashData flashData;

uint32 pwm_duty_init[1]={0};
uint32 io_info[][3]={PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2,2};
BOOL bool_pwm_init=0;
uint8 luminance;	//亮度
uint32 duty;

uint8 temperature,humidity; //温度，湿度变量

bool DealWithMessagePacketState(struct espconn *espconn,uint8 *p_buffer,uint16 length)
{
	enum MessageCtrFunction messageCtrFunction;
	static union MessagePacketUnion messagePacketUnion;
	os_memcpy(messagePacketUnion.p_buff, p_buffer, MESSAGE_PACKET_SIZE);

	messageCtrFunction=messagePacketUnion.messagePacket.function_word;
	flashData.functionState=messageCtrFunction;

	os_printf("发送者：%d\r\n",messagePacketUnion.messagePacket.sender);
	os_printf("接收者：%d\r\n",messagePacketUnion.messagePacket.receiver);

	extern  struct espconn station_ptrespconn;
	remot_info *Mpremot=NULL;
	if (espconn_get_connection_info(&station_ptrespconn, &Mpremot, 0) != 0)
			os_printf("get_connection_info fail\n");
	else{
		os_memcpy(station_ptrespconn.proto.udp->remote_ip, Mpremot->remote_ip, 4);
		station_ptrespconn.proto.udp->remote_port = Mpremot->remote_port;
	}

	if(messageCtrFunction!=WIFI_CONFIG)
	if(messagePacketUnion.messagePacket.receiver!=flashData.deviceNumber)
	{
		os_printf("设备号与请求者不匹配,发送者目标设备是:%d,本设备是:%d\r\n",messagePacketUnion.messagePacket.receiver,flashData.deviceNumber);
		messagePacketUnion.messagePacket.receiver=messagePacketUnion.messagePacket.sender;	//设置接收者
		messagePacketUnion.messagePacket.sender=flashData.deviceNumber;	//发送者为本设备号
		messagePacketUnion.messagePacket.function_word=DEVICE_ERROS;	//设备号不匹配
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,5);	//发送心跳包
		
		return ;
	}

	if(messageCtrFunction!=WIFI_CONFIG)
	{
		messagePacketUnion.messagePacket.receiver=messagePacketUnion.messagePacket.sender;	//设置接收者
		messagePacketUnion.messagePacket.sender=flashData.deviceNumber;	//发送者为本设备号
	}

	switch(messageCtrFunction)
	{
	case HEARTBEAT:
		//服务器存在
		break;
	case FIND_DEVICE:
		//回复心跳包给查找者
		messagePacketUnion.messagePacket.function_word=HEARTBEAT;	//心跳包
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//发送心跳包
		os_printf("发送心跳包\r\n");
		break;
	case LIGHT:
		luminance=messagePacketUnion.messagePacket.data[0];	//亮度
		flashData.data[0]=luminance;
		
		 if(bool_pwm_init==0)
		 {
			 bool_pwm_init=1;
			 pwm_init(1000,pwm_duty_init,1,io_info);
			 pwm_set_duty(duty,0);
		 }

		if(luminance==255)	//全开
		{
			duty=22222;
			pwm_set_duty(duty,0);
			pwm_start();
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//发送确认包
			os_printf("开灯操作\r\n");
		}
		else if(luminance==0)	//全关
		{
			duty=0;
			pwm_set_duty(duty,0);
			pwm_start();
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//发送确认包
			os_printf("关灯操作\r\n");
		}
		else	//pwm功率控制
		{
			duty=(uint32)luminance*87;
			pwm_set_duty(duty,0);
			pwm_start();
			os_printf("PWM:%d\r\n",luminance);
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//发送确认包
		}

		WriteMyFlashData(&flashData,sizeof(flashData));
		break;
	case FAN:
		break;
	case HUMITURE:

		//Get_DHT11_Data(&temperature,&humidity);
		{

		pollDHTCb(NULL );
		temperature=wendu;
		humidity=shidu;
		}

		messagePacketUnion.messagePacket.data[0]=temperature;
		messagePacketUnion.messagePacket.data[1]=humidity;
		os_printf("温度:%d\t 湿度:%d\r\n",temperature,humidity);
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,7);	//发送确认包
		break;

	case WIFI_CONFIG:
		flashData.deviceNumber=messagePacketUnion.messagePacket.receiver;
		flashData.ssidLen=messagePacketUnion.messagePacket.data[0];
		memcpy(flashData.wifi_ssid,messagePacketUnion.messagePacket.data+1,flashData.ssidLen);
		

		flashData.passwordLen=messagePacketUnion.messagePacket.data[flashData.ssidLen+1];
		memcpy(flashData.wifi_password,messagePacketUnion.messagePacket.data+flashData.ssidLen+2,flashData.passwordLen);

		//主动加入字符串结束符
		flashData.wifi_ssid[flashData.ssidLen]='\0';
		flashData.ssidLen++;
		flashData.wifi_password[flashData.passwordLen]='\0';
		flashData.passwordLen++;

		flashData.flag_init=1;
		os_printf("device number:%d\r\n",flashData.deviceNumber);
		os_printf("ssidLen:%d\r\n",flashData.ssidLen);
        os_printf("wifi_ssid:%s\r\n",flashData.wifi_ssid);
		os_printf("passWordLen:%d\r\n",flashData.passwordLen);
        os_printf("wifi_password:%s\r\n",flashData.wifi_password);
        os_printf("function World:%d\r\n",flashData.functionState);
        os_printf("function data:%d\r\n",flashData.data);
        os_printf("flag:%d\r\n",flashData.flag_init);  

		WriteMyFlashData(&flashData,sizeof(flashData));
		system_restart();
		break;
	default:
		break;
	}
}
