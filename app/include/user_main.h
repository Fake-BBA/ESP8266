#ifndef USER_MAIN_H__
#define USER_MAIN_H__

#include "driver/uart.h"
#include "osapi.h"
#include "gpio.h"
#include "spi_flash.h"
#include "eagle_soc.h"
#include "mem.h"	//内存操作相关
#include "user_interface.h"	//
#include "espconn.h"		//网络接口相关
#include "task.h"
#include "MyFlash.h"


#if (SPI_FLASH_SIZE_MAP == 0) 
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 1)
#define SYSTEM_PARTITION_OTA_SIZE
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000		//即user1,user2的大小
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0xfd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000		//即user1,user2的大小（424kb）
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000		//user2起始地址，前面一个扇区是预留的，大小等于boot
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000	//用于系统自动保存校正后的RF参数（4KB）
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000	//默认RF参数区（4KB）
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000	//系统参数区（12KB，带RF默认参数区共16KB）
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000		//用户参数，可放在任意未被占用的Flash
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0xfc000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0xfc000
#else
#error "The flash map is not supported"
#endif

#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM                SYSTEM_PARTITION_CUSTOMER_BEGIN

void ICACHE_FLASH_ATTR system_init_done();
void ICACHE_FLASH_ATTR udp_init(struct espconn* p_ptrespconn,int local_port);
void ICACHE_FLASH_ATTR scan_done(void *arg, STATUS status);
void ICACHE_FLASH_ATTR Wifi_conned(void *arg);
void ICACHE_FLASH_ATTR AP_UDP_Init();


#endif