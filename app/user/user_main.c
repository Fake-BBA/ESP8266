/*
 * 此工程由BBA(401131019@qq.com编写，用以完成课程实践
 */

#include "driver/uart.h"
#include "osapi.h"
#include "gpio.h"
#include "spi_flash.h"
#include "eagle_soc.h"
#include "mem.h"	//内存操作相关
#include "user_interface.h"
#include "espconn.h"		//网络接口相关

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#else
#error "The flash map is not supported"
#endif

static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 						0x0, 												0x1000},
    { SYSTEM_PARTITION_OTA_1,   						0x1000, 											SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   						SYSTEM_PARTITION_OTA_2_ADDR, 						SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  						SYSTEM_PARTITION_RF_CAL_ADDR, 						0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 						SYSTEM_PARTITION_PHY_DATA_ADDR, 					0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 				SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 			0x3000},
};

void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]),SPI_FLASH_SIZE_MAP)) {
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}

ETSTimer connect_timer;	//定时器事件结构体

struct station_config temp_station_config;
struct softap_config temp_AP_config;

struct espconn station_ptrespconn;	//UDP imformation struct
struct espconn AP_ptrespconn;	//UDP imformation struct


void ICACHE_FLASH_ATTR system_init_done();
void ICACHE_FLASH_ATTR udp_init(struct espconn* p_ptrespconn,int local_port);
void ICACHE_FLASH_ATTR scan_done(void *arg, STATUS status);
void ICACHE_FLASH_ATTR Wifi_conned(void *arg);


void ICACHE_FLASH_ATTR
AP_UDP_Init()
{
	wifi_softap_get_config(&temp_AP_config);	//查询AP接口保存在Flash中的配置
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
	udp_init(&AP_ptrespconn,1025);	//初始化UDP

}
/*
 * UDP接收事件
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
	//初始化UDP
	p_ptrespconn->type = ESPCONN_UDP;
	p_ptrespconn->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	p_ptrespconn->proto.udp->local_port = local_port;
	espconn_regist_recvcb(p_ptrespconn, udp_event_recv);
	espconn_create(p_ptrespconn);
	os_printf("UDP init done\r\n");
	//espconn_send(p_ptrespconn,"UDP init Done!",14);	//UDP初始化完成
}

void user_init(void)
{
	system_init_done_cb(system_init_done);	//注册系统初始化完成回调函数
}


void ICACHE_FLASH_ATTR system_init_done()
{
	os_printf("System init done\r\n");

	//UART0 IO3 RX,IO1 TX,UART1 IO2 TX,IO8 RX
	uart_init(BIT_RATE_115200, BIT_RATE_115200);	//串口初始化

	wifi_set_opmode(0x03);		//设置WiFi工作模式为AP+Station并保存到Flash

	AP_UDP_Init();	//用于在任何时刻都可对ESP8266进行设置
	struct station_config stationConf;
	os_memcpy(&stationConf.ssid, "BBA-SUMDAY", 32); //0s_memcpy 内存拷贝函数 32表示拷贝前32个字符
	os_memcpy(&stationConf.password, "123123123", 64);
	wifi_station_set_config_current(&stationConf);
	if(wifi_station_connect())	//如果wifi连接成功
	{
		udp_init(&station_ptrespconn,1026);	//初始化UDP
		os_timer_setfn(&connect_timer,Wifi_conned,NULL);	//设置软件定时器和回调函数，NULL回调函数不带参数
		os_timer_arm(&connect_timer,2000,0); //每隔2s 去检测是否真的连接上，NULL代表不重复
	}
	else
	{
		os_printf("Connect Fail!\r\n");
	}
}

void ICACHE_FLASH_ATTR Wifi_conned(void *arg)
{
	static uint8 count=0;
	uint8 status;
	{
#define BBA_PARTITION_ADDR 0x3c000
#define BBA_BBA_PARTITION_SIZE_BYTES	4 //若不能整除4，则内存读取出错会导致不断重启
		uint8 myFlashData[BBA_BBA_PARTITION_SIZE_BYTES];	//必须4字节对齐
		const uint8 BBA_flag[]="BBA_";
		spi_flash_read(BBA_PARTITION_ADDR,(uint32 *)myFlashData,BBA_BBA_PARTITION_SIZE_BYTES);
		//if(strcmp(BBA_flag,myFlashData)==0)
		{
			os_printf("Get BBA_data\r\n");
			os_printf(myFlashData);
		}
		//else
		{
			//os_printf("Set BBA_data\r\n");
			spi_flash_write(BBA_PARTITION_ADDR,(uint32 *)myFlashData,BBA_BBA_PARTITION_SIZE_BYTES);
			//system_restart();
		}
	}
	
	os_timer_disarm(&connect_timer);//这里先把定时器关了，一旦连上或者超过七次没连上，就不再重复打印了。
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
	os_timer_arm(&connect_timer,2000,0);//重新打开定时器
}


