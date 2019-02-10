/*
 * task.c
 *
 *  Created on: 2019年1月31日
 *      Author: BBA
 */

#include "task.h"


uint16 deviceNumber=15;

bool DealWithMessagePacketState(struct espconn *espconn,uint8 *p_buffer,uint16 length)
{
	enum MessageCtrFunction messageCtrFunction;
	static union MessagePacketUnion messagePacketUnion;
	os_memcpy(messagePacketUnion.p_buff, p_buffer, 20);

	os_printf("发送者：%d\r\n",messagePacketUnion.messagePacket.sender);
	os_printf("接收者：%d\r\n",messagePacketUnion.messagePacket.receiver);

	messagePacketUnion.messagePacket.receiver=messagePacketUnion.messagePacket.sender;
	messagePacketUnion.messagePacket.sender=deviceNumber;	//发送者为本设备号

	messageCtrFunction=messagePacketUnion.messagePacket.function_word;
	extern  struct espconn ptrespconn;
	remot_info *Mpremot=NULL;
	if (espconn_get_connection_info(&ptrespconn, &Mpremot, 0) != 0)
			os_printf("get_connection_info fail\n");
	else{
		os_memcpy(ptrespconn.proto.udp->remote_ip, Mpremot->remote_ip, 4);
		ptrespconn.proto.udp->remote_port = Mpremot->remote_port;
	}

	switch(messageCtrFunction)
	{
	case HEARTBEAT:
		//服务器存在
		break;
	case FIND_DEVICE:
		//恢复心跳包给查找者

		messagePacketUnion.messagePacket.function_word=0;	//心跳包

		espconn_send(&ptrespconn,messagePacketUnion.p_buff,6);	//发送心跳包
		os_printf("发送心跳包\r\n");
		break;
	case LIGHT:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);	//选择引脚功能
		PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);	//屏蔽GPIO2 上拉

		uint8 luminance=messagePacketUnion.messagePacket.data[0];	//亮度
		//const uint32 one_duty=1/PWM_PERIOD * 1000 /45/200;
		if(luminance==255)	//全开
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);	//设置为高电平
			//replyPacket[6]=1;	//灯状态
			espconn_send(&ptrespconn,messagePacketUnion.p_buff,7);	//发送确认包
			os_printf("开灯操作\r\n");
		}
		else if(luminance==0)	//全关
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);	//设置为低电平
			//replyPacket[6]=0;	//灯状态
			espconn_send(&ptrespconn,messagePacketUnion.p_buff,7);	//发送确认包
			os_printf("关灯操作\r\n");
		}
		else	//pwm功率控制
		{
			os_printf("PWM:%d\r\n",luminance);
			espconn_send(&ptrespconn,messagePacketUnion.p_buff,7);	//发送确认包
		}
		break;
	case FAN:
		break;
	case HUMITURE:
		break;
	default:
		break;
	}
}
