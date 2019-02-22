#ifndef MY_FLASH_H__
#define MY_FLASH_H__

#include "driver/uart.h"
#include "osapi.h"
#include "gpio.h"
#include "spi_flash.h"
#include "eagle_soc.h"
#include "mem.h"	//内存操作相关
#include "user_interface.h"	//


#define BBA_SECTORS	250			//以第250个扇区为数据保存扇区（从第0个扇区开始计起）
#define BBA_PARTITION_ADDR BBA_SECTORS*4096	//起始地址
#define BBA_BBA_PARTITION_SIZE_BYTES	sizeof(struct BBA_FlashData) //若不能整除4，则内存读取出错会导致不断重启

//必须4字节对齐
struct BBA_FlashData{
    
    uint8 wifi_ssid[32];
    uint32 ssidLen;
    uint8 wifi_password[64];
    uint32 passwordLen;

    uint8 functionState;  //上一次保存的功能
    uint8 flag_init;        //此包是否已被初始化
    uint16 deviceNumber;    //设备号
    
    uint8 data[4];          //功能字对应的数据，若是灯则对应亮度
};

uint32 ICACHE_FLASH_ATTR ReadMyFlashData(struct BBA_FlashData *p);
uint32 ICACHE_FLASH_ATTR WriteMyFlashData(struct BBA_FlashData *p,uint32 len);

#endif