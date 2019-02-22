/*
 * task.c
 *
 *  Created on: 2019��1��31��
 *      Author: BBA
 */

#include "task.h"
extern struct BBA_FlashData flashData;

uint32 pwm_duty_init[1]={0};
uint32 io_info[][3]={PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2,2};
BOOL bool_pwm_init=0;
uint8 luminance;	//����
uint32 duty;

uint8 temperature,humidity; //�¶ȣ�ʪ�ȱ���

bool DealWithMessagePacketState(struct espconn *espconn,uint8 *p_buffer,uint16 length)
{
	enum MessageCtrFunction messageCtrFunction;
	static union MessagePacketUnion messagePacketUnion;
	os_memcpy(messagePacketUnion.p_buff, p_buffer, MESSAGE_PACKET_SIZE);

	messageCtrFunction=messagePacketUnion.messagePacket.function_word;
	flashData.functionState=messageCtrFunction;

	os_printf("�����ߣ�%d\r\n",messagePacketUnion.messagePacket.sender);
	os_printf("�����ߣ�%d\r\n",messagePacketUnion.messagePacket.receiver);

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
		os_printf("�豸���������߲�ƥ��,������Ŀ���豸��:%d,���豸��:%d\r\n",messagePacketUnion.messagePacket.receiver,flashData.deviceNumber);
		messagePacketUnion.messagePacket.receiver=messagePacketUnion.messagePacket.sender;	//���ý�����
		messagePacketUnion.messagePacket.sender=flashData.deviceNumber;	//������Ϊ���豸��
		messagePacketUnion.messagePacket.function_word=DEVICE_ERROS;	//�豸�Ų�ƥ��
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,5);	//����������
		
		return ;
	}

	if(messageCtrFunction!=WIFI_CONFIG)
	{
		messagePacketUnion.messagePacket.receiver=messagePacketUnion.messagePacket.sender;	//���ý�����
		messagePacketUnion.messagePacket.sender=flashData.deviceNumber;	//������Ϊ���豸��
	}

	switch(messageCtrFunction)
	{
	case HEARTBEAT:
		//����������
		break;
	case FIND_DEVICE:
		//�ظ���������������
		messagePacketUnion.messagePacket.function_word=HEARTBEAT;	//������
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//����������
		os_printf("����������\r\n");
		break;
	case LIGHT:
		luminance=messagePacketUnion.messagePacket.data[0];	//����
		flashData.data[0]=luminance;
		
		 if(bool_pwm_init==0)
		 {
			 bool_pwm_init=1;
			 pwm_init(1000,pwm_duty_init,1,io_info);
			 pwm_set_duty(duty,0);
		 }

		if(luminance==255)	//ȫ��
		{
			duty=22222;
			pwm_set_duty(duty,0);
			pwm_start();
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//����ȷ�ϰ�
			os_printf("���Ʋ���\r\n");
		}
		else if(luminance==0)	//ȫ��
		{
			duty=0;
			pwm_set_duty(duty,0);
			pwm_start();
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//����ȷ�ϰ�
			os_printf("�صƲ���\r\n");
		}
		else	//pwm���ʿ���
		{
			duty=(uint32)luminance*87;
			pwm_set_duty(duty,0);
			pwm_start();
			os_printf("PWM:%d\r\n",luminance);
			espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//����ȷ�ϰ�
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
		os_printf("�¶�:%d\t ʪ��:%d\r\n",temperature,humidity);
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,7);	//����ȷ�ϰ�
		break;

	case WIFI_CONFIG:
		flashData.deviceNumber=messagePacketUnion.messagePacket.receiver;
		flashData.ssidLen=messagePacketUnion.messagePacket.data[0];
		memcpy(flashData.wifi_ssid,messagePacketUnion.messagePacket.data+1,flashData.ssidLen);
		

		flashData.passwordLen=messagePacketUnion.messagePacket.data[flashData.ssidLen+1];
		memcpy(flashData.wifi_password,messagePacketUnion.messagePacket.data+flashData.ssidLen+2,flashData.passwordLen);

		//���������ַ���������
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
