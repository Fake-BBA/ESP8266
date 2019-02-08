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
	static enum MessagePacketState messagePacketState=0;
	static enum MessageCtrFunction messageCtrFunction=0;
	uint8 highTemp,lowTemp;	//临时用以类型转换
	uint8 replyPacket[MAX_REPLY_PACKET_SIZE];	//回复包缓存区
	uint16 sender;	//发送者ID
	uint16 receiver;	//接收者ID

	//将发送者的IP地址和端口变为UDP接收IP地址和地址
	extern  struct espconn ptrespconn;
	remot_info *Mpremot=0;
	if (espconn_get_connection_info(&ptrespconn, &Mpremot, 0) != 0)
			os_printf("get_connection_info fail\n");
	else{
		os_memcpy(ptrespconn.proto.udp->remote_ip, Mpremot->remote_ip, 4);
		ptrespconn.proto.udp->remote_port = Mpremot->remote_port;
	}

	switch(messagePacketState)
	{
	case FIRST_BYTE:
		if(*p_buffer!=0xBB)
		{
			os_printf("不是本系统的包\r\n");
			return FALSE;	//不是本系统范围内的包
		}
		p_buffer++;
		//break;
	case SENDER:
		sender=(uint8)*p_buffer<<8;
		sender |=(uint8)*(p_buffer+1);
		p_buffer+=2;
		os_printf("发送者：%d\r\n",sender);
		//break;
	case RECEIVER:
		//
		receiver=(uint8)*p_buffer<<8;
		receiver |=(uint8)*(p_buffer+1);
		p_buffer+=2;
		os_printf("接收者：%d\r\n",receiver);
		if(!((deviceNumber==receiver)||(receiver==0xffff)))
		{
			os_printf("不是本设备的包\r\n");
			return FALSE;
		}
		//break;
	case FUNCTION:
		messageCtrFunction=*p_buffer;
		p_buffer++;
		switch(messageCtrFunction)
		{
		case HEARTBEAT:
			//服务器存在
			break;
		case FIND_DEVICE:
			//发送本设备给查找者
			replyPacket[1]=deviceNumber>>8;	//先传低位
			replyPacket[2]=deviceNumber;
			replyPacket[3]=sender>>8;
			replyPacket[4]=sender;
			replyPacket[5]=0;	//心跳包
			espconn_send(&ptrespconn,replyPacket,6);	//发送心跳包
			os_printf("发送心跳包\r\n");
			//os_printf(replyPacket);
			break;
		case LIGHT:
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);	//选择引脚功能
			PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);	//屏蔽GPIO2 上拉
			replyPacket[0]=0xBB;
			replyPacket[1]=deviceNumber>>8;	//先传低位
			replyPacket[2]=deviceNumber;
			replyPacket[3]=sender>>8;
			replyPacket[4]=sender;
			replyPacket[5]=LIGHT;	//灯控制
			uint8 luminance=(uint8)*p_buffer;	//亮度
			//const uint32 one_duty=1/PWM_PERIOD * 1000 /45/200;
			if(luminance==255)	//全开
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);	//设置为高电平
				replyPacket[6]=1;	//灯状态
				espconn_send(&ptrespconn,replyPacket,7);	//发送确认包
				os_printf("开灯操作\r\n");
			}
			else if(luminance==0)	//全关
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);	//设置为低电平
				replyPacket[6]=0;	//灯状态
				espconn_send(&ptrespconn,replyPacket,7);	//发送确认包
				os_printf("关灯操作\r\n");
			}
			else	//pwm功率控制
			{
				os_printf("PWM:%d\r\n",luminance);
			}
			break;
		case FAN:
			break;
		case HUMITURE:
			break;
		default:
			break;
		}
	case	DATA:
			break;
	default:
		;
	}
}
