/*
 * task.c
 *
 *  Created on: 2019年1月31日
 *      Author: BBA
 */

#include "task.h"

uint8 delay_ms(uint32 n)
{
	os_delay_us(1000);
	return 1;
}

#define DHT_PIN	GPIO_ID_PIN(2)
uint8 Get_DHT11_Data(uint8* p_temperature,uint8* p_humidity)
{
	uint32 counter;	//用来计时
	uint8 i;
	uint8 j;
	uint8 data[5];	//存储DHT11返回的5字节数据

	//注意修改此端脚。需要修改对应的GPIO寄存器
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);	//D4
	PIN_PULLUP_EN(GPIO_ID_PIN(2));	//使能GPIO2 上拉
	//禁止端口中断操作
	ets_intr_lock();

	// 设置连接DHT11的引脚输出为高，持续20毫秒
	GPIO_OUTPUT_SET(DHT_PIN, 1);
	delay_ms(20);

	// 设置连接DHT11的引脚输出为低，持续20毫秒
	GPIO_OUTPUT_SET(DHT_PIN, 0);
	delay_ms(20);

	// 设置连接DHT11的引脚设置为输入，并延时40微秒,准备读写
	GPIO_DIS_OUTPUT(DHT_PIN);	//有上拉电阻，此时为高电平
	os_delay_us(40);

	// 限时等待连接DHT11的引脚输入状态变成0，即等待获取DHT响应，如长时间未变为0则转为读取失败状态
	counter=0;
	while (GPIO_INPUT_GET(DHT_PIN) == 1) {
		if (counter >= 3) {
			os_printf("DHT11 Read Fail!");	//DHT并没有反馈
			return 0;
		}
		counter++;
		os_delay_us(1);
	}
	counter=0;
	i=0;

	// 限时等待连接DHT11的引脚输入状态变成1，即等待获取DHT响应，如长时间未变为1则转为读取失败状态
	while (GPIO_INPUT_GET(DHT_PIN) == 0) {
		if (i >= 3) {
			os_printf("DHT11 Read Fail!");	//DHT并没有反馈
			return 0;
		}
		counter++;
		os_delay_us(1);
	}

	//这时反馈信号已结束
	// 通过循环读取DHT返回的40位0、1数据

	for (i = 0; i < 5; i++)
	{
		for(j=0;j<8;j++)
		{
			counter = 0;
			//等待50us低电平（开始信号结束）
			while (GPIO_INPUT_GET(DHT_PIN) == 0)
			{
				counter++;
				os_delay_us(1);
				if (counter >= 100)
				{
					os_printf("DHT11 Read Fail!");
					return 0;
				}
			}

			counter = 0;
			//0：高电平持续26-28us 1：高电平持续70us
			while (GPIO_INPUT_GET(DHT_PIN) == 1)
			{
				counter++;
				os_delay_us(1);
				if (counter >= 100)
				{
					os_printf("DHT11 Read Fail!");
					return 0;
				}
			}

			if(counter<30&&counter>25)
			{
				data[i]&=~(1<<j);
			}
			else
			if(counter<100)
			{
				data[i] |=1<<j;
			}
		}
	}
	if(data[1]==0&&data[3]==0&&(data[0]+data[2]==data[4]))
	{
		os_printf("正确读取DHT11数据\r\n");
		*p_temperature=data[0];
		*p_humidity=data[2];
	}
	else
	{
		os_printf("DHT11数据读取错误\r\n");
	}
}
uint16 deviceNumber=15;

bool DealWithMessagePacketState(struct espconn *espconn,uint8 *p_buffer,uint16 length)
{
	enum MessageCtrFunction messageCtrFunction;
	static union MessagePacketUnion messagePacketUnion;
	os_memcpy(messagePacketUnion.p_buff, p_buffer, 20);

	os_printf("发送者：%d\r\n",messagePacketUnion.messagePacket.sender);
	os_printf("接收者：%d\r\n",messagePacketUnion.messagePacket.receiver);

	messagePacketUnion.messagePacket.receiver=messagePacketUnion.messagePacket.sender;	//设置接收者
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

	 uint8 luminance=messagePacketUnion.messagePacket.data[0];	//亮度
	 uint32 duty;

	 uint8 temperature,humidity; //温度，湿度变量
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
		 if(bool_pwm_init==0)
		 {
			 bool_pwm_init=1;
			 pwm_init(1000,pwm_duty_init,1,io_info);
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
		break;
	case FAN:
		break;
	case HUMITURE:

		Get_DHT11_Data(&temperature,&humidity);
		messagePacketUnion.p_buff[0]=temperature;
		messagePacketUnion.p_buff[1]=humidity;
		os_printf("温度:%d\t 湿度:%d\r\n",temperature,humidity);
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,7);	//发送确认包
		break;
	default:
		break;
	}
}
