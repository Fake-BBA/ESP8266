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
	enum MessageCtrFunction messageCtrFunction;
	static union MessagePacketUnion messagePacketUnion;
	os_memcpy(messagePacketUnion.p_buff, p_buffer, 20);

	os_printf("�����ߣ�%d\r\n",messagePacketUnion.messagePacket.sender);
	os_printf("�����ߣ�%d\r\n",messagePacketUnion.messagePacket.receiver);

	messagePacketUnion.messagePacket.receiver=messagePacketUnion.messagePacket.sender;
	messagePacketUnion.messagePacket.sender=deviceNumber;	//������Ϊ���豸��

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
		//����������
		break;
	case FIND_DEVICE:
		//�ָ���������������

		messagePacketUnion.messagePacket.function_word=0;	//������

		espconn_send(&ptrespconn,messagePacketUnion.p_buff,6);	//����������
		os_printf("����������\r\n");
		break;
	case LIGHT:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);	//ѡ�����Ź���
		PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);	//����GPIO2 ����

		uint8 luminance=messagePacketUnion.messagePacket.data[0];	//����
		//const uint32 one_duty=1/PWM_PERIOD * 1000 /45/200;
		if(luminance==255)	//ȫ��
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);	//����Ϊ�ߵ�ƽ
			//replyPacket[6]=1;	//��״̬
			espconn_send(&ptrespconn,messagePacketUnion.p_buff,7);	//����ȷ�ϰ�
			os_printf("���Ʋ���\r\n");
		}
		else if(luminance==0)	//ȫ��
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);	//����Ϊ�͵�ƽ
			//replyPacket[6]=0;	//��״̬
			espconn_send(&ptrespconn,messagePacketUnion.p_buff,7);	//����ȷ�ϰ�
			os_printf("�صƲ���\r\n");
		}
		else	//pwm���ʿ���
		{
			os_printf("PWM:%d\r\n",luminance);
			espconn_send(&ptrespconn,messagePacketUnion.p_buff,7);	//����ȷ�ϰ�
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
