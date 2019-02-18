/*
 * Dri_DHT11.h
 *
 *  Created on: 2019年2月11日
 *      Author: BBA
 */

#ifndef APP_INCLUDE_DRIVER_DRI_DHT11_H_
#define APP_INCLUDE_DRIVER_DRI_DHT11_H_


#include "driver/uart.h"
#include "osapi.h"
#include "gpio.h"
#include "eagle_soc.h"
#include "mem.h"	//内存操作相关
#include "user_interface.h"
#include "espconn.h"		//网络接口相关
#include "BBA_Typedef.h"

uint8 Get_DHT11_Data(uint8* p_temperature,uint8* p_humidity);

enum sensor_type{
	SENSOR_DHT11=0
};

void DHTInit(enum sensor_type sensor_type);
void ICACHE_FLASH_ATTR pollDHTCb(void * arg);
extern uint8 wendu,shidu;
#endif /* APP_INCLUDE_DRIVER_DRI_DHT11_H_ */
