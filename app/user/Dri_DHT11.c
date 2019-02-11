/*
 * Dri_DHT11.c
 *
 *  Created on: 2019��2��11��
 *      Author: BBA
 */
#include "Dri_DHT11.h"

uint8 My_delay_ms(uint32 n)
{
	uint8 i;
	for(i=0;i<n;i++)
	{
		os_delay_us(1000);
	}
	return 1;
}

uint8 wendu, shidu;

//����ѭ���������������
#define MAXTIMINGS 10000

//����ѭ���������������
#define DHT_MAXCOUNT 32000

//��ȡDHT��ƽʱ���ķֽ�ֵ�����ڴ�ʱ������Ӧ��ֵΪΪ0�����ڴ�ʱ��Ϊ1���μ�DHTģ��˵��
#define BREAKTIME 32

//����DHTģ�����Esp8266���ĸ����ţ�������gpio0Ϊ������ҿ������и�����Ӳ������һ�¡�
#define DHT_PIN 5


//����ö�����͵Ĵ���������
enum sensor_type SENSOR=0;

//���ݴ�DHTģ���ϻ�ȡ������ֵ������ʪ��ֵ�ĺ�����DHT11��DHT22������ֵ��ʪ��ֵ�Ĺ�ϵ���ĸ��Ե�ģ��˵�������Ｔ�Ǹ��ݸ���ģ��Ĺ������ԣ��������㷽��
//DHT11��ʪ��ֵΪǰ��λ���壬��¼��data[0]��
static inline float scale_humidity(int *data) {
	if (SENSOR == SENSOR_DHT11) {
		return data[0];
	} else {
		float humidity = data[0] * 256 + data[1];
		return humidity /= 10;
	}
}

//���ݴ�DHTģ���ϻ�ȡ������ֵ�������¶�ֵ�ĺ�����DHT11��DHT22������ֵ���¶�ֵ�Ĺ�ϵ���ĸ��Ե�ģ��˵�������Ｔ�Ǹ��ݸ���ģ��Ĺ������ԣ��������㷽��
//DHT11���¶�ֵΪ�������λ���壬��¼��data[3]��
static inline float scale_temperature(int *data) {
	if (SENSOR == SENSOR_DHT11) {
		return data[2];
	} else {
		float temperature = data[2] & 0x7f;
		temperature *= 256;
		temperature += data[3];
		temperature /= 10;
		if (data[2] & 0x80)
			temperature *= -1;
		return temperature;
	}
}

//������ʱ��������ϵͳ΢����ʱ������1000��
static inline void delay_ms(int sleep) {
	os_delay_us(1000 * sleep);
}

struct sensor_reading{
	char *source;
	uint8 success;
	double temperature;
	double humidity;
};
//����һ����ȡDHTģ�����ֵ�ı���������ʼ��ΪDHT11ģ��Ͷ�ȡ�ɹ�����־Ϊ��
static struct sensor_reading reading = { .source = "DHT11", .success = 0 };

//��ȡDHTģ������  static
void ICACHE_FLASH_ATTR pollDHTCb(void * arg) {
	//��������
	int counter = 0;
	//����״̬��¼����
	int laststate = 1;
	//ѭ�����Ʊ���
	int i = 0;
	//��ȡ��������λ������
	int bits_in = 0;

	//���ڼ�¼��ȡ�������ݵ�����
	int data[100];
	//��ʼ��ǰ5λΪ0���ֱ��¼ʪ��������ʪ��С�����¶��������¶�С����У���
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;

	DHTInit(0);
	//��ֹ�жϲ���
	ets_intr_lock();

	//����DHT�Ķ�ȡʱ�򣬾���ʱ�����DHTģ��˵��
	// ��������DHT11���������Ϊ�ߣ�����20����
	GPIO_OUTPUT_SET(DHT_PIN, 1);
	delay_ms(20);

	// ��������DHT11���������Ϊ�ͣ�����20����
	GPIO_OUTPUT_SET(DHT_PIN, 0);
	delay_ms(20);

	// ��������DHT11����������Ϊ���룬����ʱ40΢��
	GPIO_DIS_OUTPUT(DHT_PIN);
	os_delay_us(40);

	// ��ʱ�ȴ�����DHT11����������״̬���0�����ȴ���ȡDHT��Ӧ���糤ʱ��δ��Ϊ0��תΪ��ȡʧ��״̬
	while (GPIO_INPUT_GET(DHT_PIN) == 1 && i < DHT_MAXCOUNT) {
		if (i >= DHT_MAXCOUNT) {
			goto fail;
		}
		i++;
	}

	// ͨ��ѭ����ȡDHT���ص�40λ0��1����
	for (i = 0; i < MAXTIMINGS; i++) {
		// ÿ��ѭ����������
		counter = 0;
		//�����������״̬û�иı䣬�������1����ʱ1΢������������1000��Ҳ����˵ͬһ����״̬����1000΢�����ϣ���������״̬�ı䣬�����whileѭ��
		while (GPIO_INPUT_GET(DHT_PIN) == laststate) {
			counter++;
			os_delay_us(1);
			if (counter == 1000)
				break;
		}
		//��¼���µ�����״̬
		laststate = GPIO_INPUT_GET(DHT_PIN);
		//�����������1000������forѭ��
		if (counter == 1000)
			break;

		// ���ݶ�ȡʱ��DHT��Ӧ��ʼ�󣬵��������������ĸߵ�ƽʱ�䳤����Ϊ0����1��ֵ������ÿ����һ��50΢��Ŀ�ʼ���������źź�Ϊ�������ݣ��ݴ˹��ɽ��������ȡ
		//���Ϊ����0��1���ݵ����壬���ȡֵ������ģ�鶨���ʱ��Ϊ0������Ϊ1��ͨ��λ�������յ���5��8λ���ݷֱ����data0��4�У��ֱ����ʪ��������ʪ��С�����¶��������¶�С����У��͵�ֵ
		if ((i > 3) && (i % 2 == 0)) {
			//Ĭ��Ϊ0�������ƶ�һλ�����һλ��0
			data[bits_in / 8] <<= 1;
			//�������32us��������Ϊ1����1���룬���λΪ1
			if (counter > BREAKTIME) {
				data[bits_in / 8] |= 1;
			}
			bits_in++;
		}
	}

	//�˳���ֹ�жϲ���
	ets_intr_unlock();
	//��ȡ������λ����40λ�����ʾ��ȡʧ��
	if (bits_in < 40) {
		goto fail;
	}

	//����dataǰ4λ����У��ͣ����㷽������DHTģ��˵��
	int checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;

	//�������õ���У������ȡ���Ĳ�һ�£����ʾ��ȡʧ��
	if (data[4] != checksum) {
		goto fail;
	}
	//��¼�õ����¶���Ϣ��ʪ����Ϣ�������Ϊ��ȡ�ɹ�������pollDHTCb����
	reading.temperature = scale_temperature(data);
	reading.humidity = scale_humidity(data);

	wendu = (uint8) (reading.temperature);
	shidu = (uint8) (reading.humidity);

	reading.success = 1;
	return;

	//��ȡʧ�ܺ󣬱��Ϊ��ȡʧ�ܣ�����pollDHTCb����
	fail:

	reading.success = 0;
}

//DHTģ���ʼ������һ������Ϊģ�����ͣ�Ҳ����DHT11����22���ڶ�������Ϊ��ȡ��ʪ��ֵ�ļ��ʱ�䣬��λΪ���룬����ģ��˵���������ڲ��õ���1��
void DHTInit(enum sensor_type sensor_type) {
	SENSOR = sensor_type;
	// ����GPIO0Ϊ��׼����������ӿڣ�������ʹ�ñ��GPIO�ӿڣ������ҲҪ�޸�Ϊ�ӿڵĽӿ�
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

}


#define DHT_PIN	5	//GPIO_ID_PIN(5)

uint8 Get_DHT11_Data(uint8* p_temperature,uint8* p_humidity)
{
	uint32 counter;	//������ʱ
	uint8 i;
	uint8 j;
	uint8 data[5];	//�洢DHT11���ص�5�ֽ�����

	//ע���޸Ĵ˶˽š���Ҫ�޸Ķ�Ӧ��GPIO�Ĵ���
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);	//D1
	//PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO5_U);	//����GPIO5 ����
	//PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);	//ʹ��GPIO5 ����
	//��ֹ�˿��жϲ���
	ets_intr_lock();

	// ��������DHT11���������Ϊ�ߣ�����20����
	GPIO_OUTPUT_SET(DHT_PIN, 1);
	delay_ms(20);

	// ��������DHT11���������Ϊ�ͣ�����20����
	GPIO_OUTPUT_SET(DHT_PIN, 0);
	delay_ms(20);

	//����30us
	//GPIO_OUTPUT_SET(DHT_PIN, 1);

	// ��������DHT11����������Ϊ���룬����ʱ40΢��,׼����д
	GPIO_DIS_OUTPUT(DHT_PIN);	//����Ϊ����ģʽ�������������裬��ʱΪ�ߵ�ƽ
	//PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);	//ʹ��GPIO5 ����
	//os_delay_us(40);

	// ��ʱ�ȴ�����DHT11����������״̬���0�����ȴ���ȡDHT��Ӧ���糤ʱ��δ��Ϊ0��תΪ��ȡʧ��״̬
	counter=0;
	while (GPIO_INPUT_GET(DHT_PIN) == 1)
	{
		if (counter >= 100) {	//100us�ڱ���Ҫ���յ��͵�ƽ�ź�
			os_printf("DHT11 Read Fail!\r\n");	//DHT��û�з���
			os_printf("�ȴ��͵�ƽ����ʧ��!\r\n");	//DHT��û�з���
			return 0;
		}
		counter++;
		os_delay_us(1);
	}
	os_printf("�ȴ��͵�ƽ��������ʱ��:%d us\r\n",counter);

	counter=0;
	// ��ʱ�ȴ�����DHT11����������״̬���1�����ȴ���ȡDHT��Ӧ���糤ʱ��δ��Ϊ1��תΪ��ȡʧ��״̬
	while (GPIO_INPUT_GET(DHT_PIN) == 0)
	{
		if (counter>=80) {	//�͵�ƽ�ź�������80us
			os_printf("DHT11 Read Fail!");	//DHT��û�з���
			os_printf("�ȴ��ߵ�ƽ����ʧ��!\r\n");	//DHT��û�з���
			return 0;
		}
		counter++;
		os_delay_us(1);
	}

	os_printf("�͵�ƽ��������ʱ��:%d us\r\n",counter);

	counter=0;
	// �ȴ��ߵ�ƽ����ʱ���ȥ
	while (GPIO_INPUT_GET(DHT_PIN) == 0) {
		if (counter>=80) {	//�ߵ�ƽ�ź�������80us
			os_printf("DHT11 Read Fail!");	//DHT��û�з���
			os_printf("�ȴ���ʼ�͵�ƽ�ź�ʧ��!\r\n");	//DHT��û�з���
			return 0;
		}
		counter++;
		os_delay_us(1);
	}
	os_printf("�ߵ�ƽ��������ʱ��:%d us\r\n",counter);
	//��ʱ�����ź��ѽ���
	// ͨ��ѭ����ȡDHT���ص�40λ0��1����

	for (i = 0; i < 5; i++)
	{
		for(j=0;j<8;j++)
		{
			counter = 0;
			//�ȴ�50us�͵�ƽ��ȥ����ʼ�źŽ�����
			while (GPIO_INPUT_GET(DHT_PIN) == 0)
			{
				counter++;
				os_delay_us(1);
				if (counter >= 1000)
				{
					os_printf("DHT11 Read Fail!");
					os_printf("��ʼλ�͵�ƽ��ʱ!\r\n");	//DHT��û�з���
					os_printf("��ʼλ�͵�ƽ����ʱ��:%d us\r\n",counter);
					return 0;
				}
			}
			os_printf("��ʼλ�͵�ƽ����ʱ��:%d us\r\n",counter);
			counter = 0;
			//0���ߵ�ƽ����26-28us 1���ߵ�ƽ����70us
			while (GPIO_INPUT_GET(DHT_PIN) == 1)
			{
				counter++;
				os_delay_us(1);
				if (counter >= 10000)
				{
					os_printf("DHT11 Read Fail!");
					os_printf("����λ�ߵ�ƽ��ʱ!\r\n");	//DHT��û�з���
					os_printf("����λλ�ߵ�ƽ����ʱ��:%d us\r\n",counter);
					return 0;
				}
			}

			os_printf("����λ�ߵ�ƽ����ʱ��:%d us\r\n",counter);
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
	//�˳���ֹ�жϲ���
	ets_intr_unlock();
	if(data[1]==0&&data[3]==0&&(data[0]+data[2]==data[4]))
	{
		os_printf("��ȷ��ȡDHT11����\r\n");
		*p_temperature=data[0];
		*p_humidity=data[2];
	}
	else
	{
		os_printf("DHT11���ݶ�ȡ����\r\n");
		os_printf("data[0]:%x\t data[1]:%x\t data[2]:%x\t data[3]:%x\t data[4]:%x\r\n",data[0],data[1],data[2],data[3],data[4]);
	}
}


