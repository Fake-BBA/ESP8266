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
	extern  struct espconn station_ptrespconn;
	remot_info *Mpremot=NULL;
	if (espconn_get_connection_info(&station_ptrespconn, &Mpremot, 0) != 0)
			os_printf("get_connection_info fail\n");
	else{
		os_memcpy(station_ptrespconn.proto.udp->remote_ip, Mpremot->remote_ip, 4);
		station_ptrespconn.proto.udp->remote_port = Mpremot->remote_port;
	}

	uint32 pwm_duty_init[1]={0};
	 uint32 io_info[][3]={PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2,2};

	 static BOOL bool_pwm_init=0;
	 if(bool_pwm_init==0)
	 {
		 bool_pwm_init=1;
		 pwm_init(1000,pwm_duty_init,1,io_info);
	 }

	 uint8 luminance=messagePacketUnion.messagePacket.data[0];	//亮度
	 uint32 duty;

	switch(messageCtrFunction)
	{
	case HEARTBEAT:
		//服务器存在
		break;
	case FIND_DEVICE:
		//恢复心跳包给查找者
		messagePacketUnion.messagePacket.function_word=0;	//心跳包
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//发送心跳包
		os_printf("发送心跳包\r\n");
		break;
	case LIGHT:


		if(luminance==255)	//全开
		{
			duty=22222;
			pwm_set_duty(duty,0);
			pwm_start();
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,7);	//发送确认包
			os_printf("开灯操作\r\n");
		}
		else if(luminance==0)	//全关
		{
			duty=0;
			pwm_set_duty(duty,0);
			pwm_start();
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,7);	//发送确认包
			os_printf("关灯操作\r\n");
		}
		else	//pwm功率控制
		{
			duty=(uint32)luminance*87;
			pwm_set_duty(duty,0);
			pwm_start();
			os_printf("PWM:%d\r\n",luminance);
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,7);	//发送确认包
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
