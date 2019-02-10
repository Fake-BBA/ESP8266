/*
 * task.c
 *
 *  Created on: 2019��1��31��
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
	uint32 counter;	//������ʱ
	uint8 i;
	uint8 j;
	uint8 data[5];	//�洢DHT11���ص�5�ֽ�����

	//ע���޸Ĵ˶˽š���Ҫ�޸Ķ�Ӧ��GPIO�Ĵ���
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);	//D4
	PIN_PULLUP_EN(GPIO_ID_PIN(2));	//ʹ��GPIO2 ����
	//��ֹ�˿��жϲ���
	ets_intr_lock();

	// ��������DHT11���������Ϊ�ߣ�����20����
	GPIO_OUTPUT_SET(DHT_PIN, 1);
	delay_ms(20);

	// ��������DHT11���������Ϊ�ͣ�����20����
	GPIO_OUTPUT_SET(DHT_PIN, 0);
	delay_ms(20);

	// ��������DHT11����������Ϊ���룬����ʱ40΢��,׼����д
	GPIO_DIS_OUTPUT(DHT_PIN);	//���������裬��ʱΪ�ߵ�ƽ
	os_delay_us(40);

	// ��ʱ�ȴ�����DHT11����������״̬���0�����ȴ���ȡDHT��Ӧ���糤ʱ��δ��Ϊ0��תΪ��ȡʧ��״̬
	counter=0;
	while (GPIO_INPUT_GET(DHT_PIN) == 1) {
		if (counter >= 3) {
			os_printf("DHT11 Read Fail!");	//DHT��û�з���
			return 0;
		}
		counter++;
		os_delay_us(1);
	}
	counter=0;
	i=0;

	// ��ʱ�ȴ�����DHT11����������״̬���1�����ȴ���ȡDHT��Ӧ���糤ʱ��δ��Ϊ1��תΪ��ȡʧ��״̬
	while (GPIO_INPUT_GET(DHT_PIN) == 0) {
		if (i >= 3) {
			os_printf("DHT11 Read Fail!");	//DHT��û�з���
			return 0;
		}
		counter++;
		os_delay_us(1);
	}

	//��ʱ�����ź��ѽ���
	// ͨ��ѭ����ȡDHT���ص�40λ0��1����

	for (i = 0; i < 5; i++)
	{
		for(j=0;j<8;j++)
		{
			counter = 0;
			//�ȴ�50us�͵�ƽ����ʼ�źŽ�����
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
			//0���ߵ�ƽ����26-28us 1���ߵ�ƽ����70us
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
		os_printf("��ȷ��ȡDHT11����\r\n");
		*p_temperature=data[0];
		*p_humidity=data[2];
	}
	else
	{
		os_printf("DHT11���ݶ�ȡ����\r\n");
	}
}
uint16 deviceNumber=15;

bool DealWithMessagePacketState(struct espconn *espconn,uint8 *p_buffer,uint16 length)
{
	enum MessageCtrFunction messageCtrFunction;
	static union MessagePacketUnion messagePacketUnion;
	os_memcpy(messagePacketUnion.p_buff, p_buffer, 20);

	os_printf("�����ߣ�%d\r\n",messagePacketUnion.messagePacket.sender);
	os_printf("�����ߣ�%d\r\n",messagePacketUnion.messagePacket.receiver);

	messagePacketUnion.messagePacket.receiver=messagePacketUnion.messagePacket.sender;	//���ý�����
	messagePacketUnion.messagePacket.sender=deviceNumber;	//������Ϊ���豸��

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

	 uint8 luminance=messagePacketUnion.messagePacket.data[0];	//����
	 uint32 duty;

	 uint8 temperature,humidity; //�¶ȣ�ʪ�ȱ���
	switch(messageCtrFunction)
	{
	case HEARTBEAT:
		//����������
		break;
	case FIND_DEVICE:
		//�ָ���������������
		messagePacketUnion.messagePacket.function_word=0;	//������
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,6);	//����������
		os_printf("����������\r\n");
		break;
	case LIGHT:
		 if(bool_pwm_init==0)
		 {
			 bool_pwm_init=1;
			 pwm_init(1000,pwm_duty_init,1,io_info);
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
		break;
	case FAN:
		break;
	case HUMITURE:

		Get_DHT11_Data(&temperature,&humidity);
		messagePacketUnion.p_buff[0]=temperature;
		messagePacketUnion.p_buff[1]=humidity;
		os_printf("�¶�:%d\t ʪ��:%d\r\n",temperature,humidity);
		espconn_send(&station_ptrespconn,messagePacketUnion.p_buff,7);	//����ȷ�ϰ�
		break;
	default:
		break;
	}
}
