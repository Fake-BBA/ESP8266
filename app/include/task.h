/*
 * task.h
 *
 *  Created on: 2019��1��31��
 *      Author: BBA
 */

#ifndef APP_INCLUDE_TASK_H_
#define APP_INCLUDE_TASK_H_

#include "BBA_Typedef.h"
#include "driver/uart.h"
#include "osapi.h"
#include "gpio.h"
#include "eagle_soc.h"
#include "mem.h"	//�ڴ�������
#include "user_interface.h"
#include "espconn.h"		//����ӿ����
#include "pwm.h"

#define MAX_REPLY_PACKET_SIZE	21
#define PWM_HZ	50
#define PWM_PERIOD	1/PWM_HZ	//��λus
#define CHANNEL	1
#define CHANNEL_NUM	1
enum MessagePacketState{
	FIRST_BYTE=0,
	SENDER,
	RECEIVER,
	FUNCTION,
	DATA
};
enum MessageCtrFunction{
	HEARTBEAT=0,
	FIND_DEVICE,
	LIGHT,
	FAN,
	HUMITURE
};

struct MessagePacketStruct{
	uint8 systemWord;
	uint16 sender;
	uint16 receiver;
	uint8 function_word;
	uint8 data[15];
};
union MessagePacketUnion{
	struct MessagePacketStruct messagePacket;
	uint8 p_buff[21];	//Э��涨�������Ϊ21
};

extern uint16 deviceNumber;
bool DealWithMessagePacketState(struct espconn *espconn,uint8 *p_buffer,uint16 length);



#endif /* APP_INCLUDE_TASK_H_ */
