/*
 * �˹�����BBA(401131019@qq.com��д��������ɿγ�ʵ��
 */
#include "user_main.h"

uint32 priv_param_start_sec;

static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 						0x0, 												0x1000},	//bootռһ������
    { SYSTEM_PARTITION_OTA_1,   						0x1000, 											SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   						SYSTEM_PARTITION_OTA_2_ADDR, 						SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  						SYSTEM_PARTITION_RF_CAL_ADDR, 						0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 						SYSTEM_PARTITION_PHY_DATA_ADDR, 					0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 				SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 			0x3000},
    { SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM,             SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR,          0x1000},
};

void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]),SPI_FLASH_SIZE_MAP)) {
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}


ETSTimer connect_timer;	//��ʱ���¼��ṹ��

struct station_config temp_station_config;
struct softap_config temp_AP_config;

struct espconn station_ptrespconn;	//UDP imformation struct
struct espconn AP_ptrespconn;	//UDP imformation struct


/*
 * UDP�����¼�
 */
void ICACHE_FLASH_ATTR
udp_event_recv(void *arg, char *pusrdata, unsigned short length)
{
	DealWithMessagePacketState(arg,pusrdata,length);
	//uart0_tx_buffer(pusrdata,length);
    return;
}

void ICACHE_FLASH_ATTR udp_init(struct espconn* p_ptrespconn,int local_port)
{
	//��ʼ��UDP
	p_ptrespconn->type = ESPCONN_UDP;
	p_ptrespconn->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	p_ptrespconn->proto.udp->local_port = local_port;
	espconn_regist_recvcb(p_ptrespconn, udp_event_recv);
	espconn_create(p_ptrespconn);
	os_printf("UDP init done\r\n");
	//espconn_send(p_ptrespconn,"UDP init Done!",14);	//UDP��ʼ�����
}

void user_init(void)
{
	system_init_done_cb(system_init_done);	//ע��ϵͳ��ʼ����ɻص�����
}

struct BBA_FlashData flashData;
void ICACHE_FLASH_ATTR system_init_done()
{
	os_printf("System init done\r\n");
	
	//UART0 IO3 RX,IO1 TX,UART1 IO2 TX,IO8 RX
	uart_init(BIT_RATE_115200, BIT_RATE_115200);	//���ڳ�ʼ��

	wifi_set_opmode(0x03);		//����WiFi����ģʽΪAP+Station�����浽Flash

	AP_UDP_Init();	//�������κ�ʱ�̶��ɶ�ESP8266��������

	ReadMyFlashData(&flashData);

	struct station_config stationConf;
	//os_memcpy(&stationConf.ssid, "BBA-SUMDAY", 32); //0s_memcpy �ڴ濽������ 32��ʾ����ǰ32���ַ�
	//os_memcpy(&stationConf.password, "123123123", 64);
	//wifi_station_set_config_current(&stationConf);

	if(flashData.flag_init==1)
	{
		os_memcpy(&stationConf.ssid, flashData.wifi_ssid, flashData.ssidLen); //0s_memcpy �ڴ濽������ 32��ʾ����ǰ32���ַ�
		os_memcpy(&stationConf.password, flashData.wifi_password, flashData.passwordLen);
		wifi_station_set_config_current(&stationConf);
		if(wifi_station_connect())	//���wifi���ӳɹ�
		{
			udp_init(&station_ptrespconn,1026);	//��ʼ��UDP
			os_printf("wifi connecting!");
			os_timer_setfn(&connect_timer,Wifi_conned,NULL);	//���������ʱ���ͻص�������NULL�ص�������������
			os_timer_arm(&connect_timer,1000,0); //ÿ��2s ȥ����Ƿ���������ϣ�NULL�����ظ�
		}
		else
		{
			os_printf("Connect Fail!\r\n");
		}
	}
	else
	{
		os_printf("wifi havn't set\r\n");
	}
}

void ICACHE_FLASH_ATTR AP_UDP_Init()
{
	wifi_softap_get_config(&temp_AP_config);	//��ѯAP�ӿڱ�����Flash�е�����
	if(temp_AP_config.ssid[0]!='B')
	{
		os_memset(temp_AP_config.ssid, 0, 32);
		os_memset(temp_AP_config.password, 0, 64);
		os_memcpy(temp_AP_config.ssid, "BBA's_AP", 8);
		os_memcpy(temp_AP_config.password, "123123123", 9);
		temp_AP_config.authmode = AUTH_WPA_WPA2_PSK;
		temp_AP_config.ssid_len = 8;// or its actual length
		temp_AP_config.max_connection = 4; // how many stations can connect to ESP8266 softAP at most.
		wifi_softap_set_config(&temp_AP_config);
		os_printf("AP Set Sucess!");
	}
	udp_init(&AP_ptrespconn,1025);	//��ʼ��UDP
}

void ICACHE_FLASH_ATTR Wifi_conned(void *arg)
{
	static uint8 count=0;
	uint8 status;
	
	os_timer_disarm(&connect_timer);//�����ȰѶ�ʱ�����ˣ�һ�����ϻ��߳����ߴ�û���ϣ��Ͳ����ظ���ӡ�ˡ�
	count++;

	status = wifi_station_get_connect_status();
	if(status == STATION_GOT_IP)
	{
		os_printf("WIFI was Connected!\r\n");
		return;
	}
	else
	{
		if(count >= 7)
		{
			os_printf("WIFI Connect Fail\r\n");
			return;
		}
	}
	os_timer_arm(&connect_timer,2000,0);//���´򿪶�ʱ��
}