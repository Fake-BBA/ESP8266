/*
 * 此工程由BBA(401131019@qq.com编写，用以完成课程实践
 */

#include "driver/uart.h"
#include "osapi.h"
#include "gpio.h"
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

void ICACHE_FLASH_ATTR gpio_init()
{
	//IO2 D4
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO12);	//选择引脚功能
	//PIN_PULLUP_DIS(GPIO_ID_PIN(2));	//屏蔽GPIO2 上拉
	//PIN_PULLUP_EN(GPIO_ID_PIN(2));	//使能GPIO2 上拉
	//GPIO_DIS_OUTPUT(GPIO_ID_PIN(2)) ;	//设置为输入模式
	//GPIO_INPUT_GET(GPIO_ID_PIN(2));	//获取GPIO电平

	//GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);	//设置为高电平
	os_printf("GPIO init done\r\n");
}

struct station_config temp_station_config;
struct softap_config temp_AP_config;

struct espconn ptrespconn;	//UDP imformation struct



void ICACHE_FLASH_ATTR system_init_done();
void ICACHE_FLASH_ATTR scan_done(void *arg, STATUS status);
void ICACHE_FLASH_ATTR Wifi_conned(void *arg);

/*
 * UDP接收事件
 */
LOCAL void ICACHE_FLASH_ATTR
udp_event_recv(void *arg, char *pusrdata, unsigned short length)
{
	DealWithMessagePacketState(arg,pusrdata,length);
	//uart0_tx_buffer(pusrdata,length);
    return;
}

void ICACHE_FLASH_ATTR udp_init()
{
	//初始化UDP
	ptrespconn.type = ESPCONN_UDP;
	ptrespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	ptrespconn.proto.udp->local_port = 1025;
	espconn_regist_recvcb(&ptrespconn, udp_event_recv);
	espconn_create(&ptrespconn);
	os_printf("UDP init done\r\n");
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
	//gpio_init();	//GPIO初始化
	//system_set_os_print (1);	//0关闭，1开启系统打印
	if(wifi_get_opmode_default()!=0x01) //查询保存在Flash中的WiFi工作模式设置
	{
		wifi_set_opmode(0x01);		//设置WiFi工作模式为AP+Station并保存到Flash
		//system_restart();			//0.9版本之后不用重启系统，即时生效
	}
	wifi_station_scan(NULL,scan_done);	//扫描所有wifi热点
}

void ICACHE_FLASH_ATTR scan_done(void *arg, STATUS status)
{
	os_printf("WIFI Scan done\r\n");
	uint8 ssid[33];
	struct station_config stationConf;

	if (status == OK)
	{
		struct bss_info *bss_link = (struct bss_info *)arg; //链表指针
		bss_link = bss_link->next.stqe_next;//ignore first

		while (bss_link != NULL)
		{
			os_memset(ssid, 0, 33); //memset 初始化数组
			if (os_strlen(bss_link->ssid) <= 32)
			{
				os_memcpy(ssid, bss_link->ssid, os_strlen(bss_link->ssid));
			}
			else
			{
				os_memcpy(ssid, bss_link->ssid, 32);
			}
			os_printf("+CWLAP:(%d,\"%s\",%d,\""MACSTR"\",%d)\r\n",
			bss_link->authmode, ssid, bss_link->rssi,
			MAC2STR(bss_link->bssid),bss_link->channel);

			bss_link = bss_link->next.stqe_next;
		}
		//struct station_config stationConf;
		os_memcpy(&stationConf.ssid, "BBA-SUMDAY", 32); //0s_memcpy 内存拷贝函数 32表示拷贝前32个字符
		os_memcpy(&stationConf.password, "123123123", 64);
		wifi_station_set_config_current(&stationConf);
		if(wifi_station_connect())	//如果wifi连接成功
		{
			udp_init();	//初始化UDP
		}
		else
		{
			os_printf("Connect Fail!\r\n");
		}
		os_timer_setfn(&connect_timer,Wifi_conned,NULL);	//设置软件定时器和回调函数，NULL回调函数不带参数
		os_timer_arm(&connect_timer,2000,0); //每隔2s 去检测是否真的连接上，NULL代表不重复
		//os_timer_disarm(&connect_timer);	//这里先把定时器关了
	}
	else
	{
		os_printf("Scan None!\r\n");
	}

}

void ICACHE_FLASH_ATTR Wifi_conned(void *arg)
{
	static uint8 count=0;
	uint8 status;

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
			return;//if忘了包含return，然后导致不会打印上面这两个。。
		}
	}
	os_timer_arm(&connect_timer,2000,0);//重新打开定时器
}


