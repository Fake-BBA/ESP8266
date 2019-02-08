/*
 * task.c
 *
 *  Created on: 2019��1��31��
 *      Author: BBA
 */

#include "task.h"


uint16 deviceNumber=15;

bool DealWithMessagePacketState(struct espconn *espconn,uint8 *p_buffer,uint16 length)
{
	static enum MessagePacketState messagePacketState=0;
	static enum MessageCtrFunction messageCtrFunction=0;
	uint8 highTemp,lowTemp;	//��ʱ��������ת��
	uint8 replyPacket[MAX_REPLY_PACKET_SIZE];	//�ظ���������
	uint16 sender;	//������ID
	uint16 receiver;	//������ID

	//�������ߵ�IP��ַ�Ͷ˿ڱ�ΪUDP����IP��ַ�͵�ַ
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
			os_printf("���Ǳ�ϵͳ�İ�\r\n");
			return FALSE;	//���Ǳ�ϵͳ��Χ�ڵİ�
		}
		p_buffer++;
		//break;
	case SENDER:
		sender=(uint8)*p_buffer<<8;
		sender |=(uint8)*(p_buffer+1);
		p_buffer+=2;
		os_printf("�����ߣ�%d\r\n",sender);
		//break;
	case RECEIVER:
		//
		receiver=(uint8)*p_buffer<<8;
		receiver |=(uint8)*(p_buffer+1);
		p_buffer+=2;
		os_printf("�����ߣ�%d\r\n",receiver);
		if(!((deviceNumber==receiver)||(receiver==0xffff)))
		{
			os_printf("���Ǳ��豸�İ�\r\n");
			return FALSE;
		}
		//break;
	case FUNCTION:
		messageCtrFunction=*p_buffer;
		p_buffer++;
		switch(messageCtrFunction)
		{
		case HEARTBEAT:
			//����������
			break;
		case FIND_DEVICE:
			//���ͱ��豸��������
			replyPacket[1]=deviceNumber>>8;	//�ȴ���λ
			replyPacket[2]=deviceNumber;
			replyPacket[3]=sender>>8;
			replyPacket[4]=sender;
			replyPacket[5]=0;	//������
			espconn_send(&ptrespconn,replyPacket,6);	//����������
			os_printf("����������\r\n");
			//os_printf(replyPacket);
			break;
		case LIGHT:
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);	//ѡ�����Ź���
			PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);	//����GPIO2 ����
			replyPacket[0]=0xBB;
			replyPacket[1]=deviceNumber>>8;	//�ȴ���λ
			replyPacket[2]=deviceNumber;
			replyPacket[3]=sender>>8;
			replyPacket[4]=sender;
			replyPacket[5]=LIGHT;	//�ƿ���
			uint8 luminance=(uint8)*p_buffer;	//����
			//const uint32 one_duty=1/PWM_PERIOD * 1000 /45/200;
			if(luminance==255)	//ȫ��
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);	//����Ϊ�ߵ�ƽ
				replyPacket[6]=1;	//��״̬
				espconn_send(&ptrespconn,replyPacket,7);	//����ȷ�ϰ�
				os_printf("���Ʋ���\r\n");
			}
			else if(luminance==0)	//ȫ��
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);	//����Ϊ�͵�ƽ
				replyPacket[6]=0;	//��״̬
				espconn_send(&ptrespconn,replyPacket,7);	//����ȷ�ϰ�
				os_printf("�صƲ���\r\n");
			}
			else	//pwm���ʿ���
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
