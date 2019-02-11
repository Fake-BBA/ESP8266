/*
 * Dri_DHT11.c
 *
 *  Created on: 2019年2月11日
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

//定义循环操作的最高上限
#define MAXTIMINGS 10000

//定义循环操作的最高上限
#define DHT_MAXCOUNT 32000

//读取DHT电平时长的分界值，低于此时长其响应数值为为0，高于此时长为1，参见DHT模块说明
#define BREAKTIME 32

//定义DHT模块接入Esp8266的哪个引脚，本例以gpio0为例，大家可以自行更改与硬件连接一致。
#define DHT_PIN 5


//定义枚举类型的传感器变量
enum sensor_type SENSOR=0;

//根据从DHT模块上获取的脉冲值，计算湿度值的函数，DHT11与DHT22的脉冲值与湿度值的关系参阅各自的模块说明，这里即是根据各自模块的固有属性，给出计算方法
//DHT11的湿度值为前八位脉冲，记录在data[0]中
static inline float scale_humidity(int *data) {
	if (SENSOR == SENSOR_DHT11) {
		return data[0];
	} else {
		float humidity = data[0] * 256 + data[1];
		return humidity /= 10;
	}
}

//根据从DHT模块上获取的脉冲值，计算温度值的函数，DHT11与DHT22的脉冲值与温度值的关系参阅各自的模块说明，这里即是根据各自模块的固有属性，给出计算方法
//DHT11的温度值为第三组八位脉冲，记录在data[3]中
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

//毫秒延时函数，即系统微妙延时函数的1000倍
static inline void delay_ms(int sleep) {
	os_delay_us(1000 * sleep);
}

struct sensor_reading{
	char *source;
	uint8 success;
	double temperature;
	double humidity;
};
//定义一个读取DHT模块各项值的变量，并初始化为DHT11模块和读取成功与否标志为假
static struct sensor_reading reading = { .source = "DHT11", .success = 0 };

//读取DHT模块数据  static
void ICACHE_FLASH_ATTR pollDHTCb(void * arg) {
	//计数变量
	int counter = 0;
	//输入状态记录变量
	int laststate = 1;
	//循环控制变量
	int i = 0;
	//获取到的数据位的数量
	int bits_in = 0;

	//用于记录获取脉冲数据的数组
	int data[100];
	//初始化前5位为0，分别记录湿度整数、湿度小数、温度整数、温度小数和校验和
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;

	DHTInit(0);
	//禁止中断操作
	ets_intr_lock();

	//构建DHT的读取时序，具体时序参阅DHT模块说明
	// 设置连接DHT11的引脚输出为高，持续20毫秒
	GPIO_OUTPUT_SET(DHT_PIN, 1);
	delay_ms(20);

	// 设置连接DHT11的引脚输出为低，持续20毫秒
	GPIO_OUTPUT_SET(DHT_PIN, 0);
	delay_ms(20);

	// 设置连接DHT11的引脚设置为输入，并延时40微秒
	GPIO_DIS_OUTPUT(DHT_PIN);
	os_delay_us(40);

	// 限时等待连接DHT11的引脚输入状态变成0，即等待获取DHT响应，如长时间未变为0则转为读取失败状态
	while (GPIO_INPUT_GET(DHT_PIN) == 1 && i < DHT_MAXCOUNT) {
		if (i >= DHT_MAXCOUNT) {
			goto fail;
		}
		i++;
	}

	// 通过循环读取DHT返回的40位0、1数据
	for (i = 0; i < MAXTIMINGS; i++) {
		// 每次循环计数归零
		counter = 0;
		//如果输入引脚状态没有改变，则计数加1并延时1微妙，如果计数超过1000，也就是说同一输入状态持续1000微妙以上，或者输入状态改变，则结束while循环
		while (GPIO_INPUT_GET(DHT_PIN) == laststate) {
			counter++;
			os_delay_us(1);
			if (counter == 1000)
				break;
		}
		//记录最新的输入状态
		laststate = GPIO_INPUT_GET(DHT_PIN);
		//如果计数超过1000，结束for循环
		if (counter == 1000)
			break;

		// 根据读取时序，DHT相应开始后，第三次脉冲跳变后的高电平时间长短则为0或者1的值，尔后每次先一个50微妙的开始发送数据信号后为具体数据，据此规律进行脉冲读取
		//如果为代表0、1数据的脉冲，则读取值，少于模块定义的时长为0，大于为1，通过位操作将收到的5组8位数据分别存入data0至4中，分别代表湿度整数、湿度小数、温度整数、温度小数及校验和的值
		if ((i > 3) && (i % 2 == 0)) {
			//默认为0，则左移动一位，最低一位补0
			data[bits_in / 8] <<= 1;
			//如果大于32us，则数据为1，和1相与，最低位为1
			if (counter > BREAKTIME) {
				data[bits_in / 8] |= 1;
			}
			bits_in++;
		}
	}

	//退出禁止中断操作
	ets_intr_unlock();
	//读取的数据位少于40位，则表示读取失败
	if (bits_in < 40) {
		goto fail;
	}

	//根据data前4位计算校验和，计算方法参阅DHT模块说明
	int checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;

	//如果计算得到的校验和与读取到的不一致，则表示读取失败
	if (data[4] != checksum) {
		goto fail;
	}
	//记录得到的温度信息与湿度信息，并标记为读取成功，结束pollDHTCb函数
	reading.temperature = scale_temperature(data);
	reading.humidity = scale_humidity(data);

	wendu = (uint8) (reading.temperature);
	shidu = (uint8) (reading.humidity);

	reading.success = 1;
	return;

	//读取失败后，标记为读取失败，结束pollDHTCb函数
	fail:

	reading.success = 0;
}

//DHT模块初始化。第一个参数为模块类型，也就是DHT11或者22，第二个参数为读取温湿度值的间隔时间，单位为毫秒，根据模块说明采样周期不得低于1秒
void DHTInit(enum sensor_type sensor_type) {
	SENSOR = sensor_type;
	// 设置GPIO0为标准的输入输出接口，如果大家使用别的GPIO接口，此语句也要修改为接口的接口
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

}


#define DHT_PIN	5	//GPIO_ID_PIN(5)

uint8 Get_DHT11_Data(uint8* p_temperature,uint8* p_humidity)
{
	uint32 counter;	//用来计时
	uint8 i;
	uint8 j;
	uint8 data[5];	//存储DHT11返回的5字节数据

	//注意修改此端脚。需要修改对应的GPIO寄存器
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);	//D1
	//PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO5_U);	//屏蔽GPIO5 上拉
	//PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);	//使能GPIO5 上拉
	//禁止端口中断操作
	ets_intr_lock();

	// 设置连接DHT11的引脚输出为高，持续20毫秒
	GPIO_OUTPUT_SET(DHT_PIN, 1);
	delay_ms(20);

	// 设置连接DHT11的引脚输出为低，持续20毫秒
	GPIO_OUTPUT_SET(DHT_PIN, 0);
	delay_ms(20);

	//拉高30us
	//GPIO_OUTPUT_SET(DHT_PIN, 1);

	// 设置连接DHT11的引脚设置为输入，并延时40微秒,准备读写
	GPIO_DIS_OUTPUT(DHT_PIN);	//设置为输入模式，若有上拉电阻，此时为高电平
	//PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);	//使能GPIO5 上拉
	//os_delay_us(40);

	// 限时等待连接DHT11的引脚输入状态变成0，即等待获取DHT响应，如长时间未变为0则转为读取失败状态
	counter=0;
	while (GPIO_INPUT_GET(DHT_PIN) == 1)
	{
		if (counter >= 100) {	//100us内必须要接收到低电平信号
			os_printf("DHT11 Read Fail!\r\n");	//DHT并没有反馈
			os_printf("等待低电平反馈失败!\r\n");	//DHT并没有反馈
			return 0;
		}
		counter++;
		os_delay_us(1);
	}
	os_printf("等待低电平反馈持续时间:%d us\r\n",counter);

	counter=0;
	// 限时等待连接DHT11的引脚输入状态变成1，即等待获取DHT响应，如长时间未变为1则转为读取失败状态
	while (GPIO_INPUT_GET(DHT_PIN) == 0)
	{
		if (counter>=80) {	//低电平信号最多持续80us
			os_printf("DHT11 Read Fail!");	//DHT并没有反馈
			os_printf("等待高电平反馈失败!\r\n");	//DHT并没有反馈
			return 0;
		}
		counter++;
		os_delay_us(1);
	}

	os_printf("低电平反馈持续时间:%d us\r\n",counter);

	counter=0;
	// 等待高电平反馈时间过去
	while (GPIO_INPUT_GET(DHT_PIN) == 0) {
		if (counter>=80) {	//高电平信号最多持续80us
			os_printf("DHT11 Read Fail!");	//DHT并没有反馈
			os_printf("等待起始低电平信号失败!\r\n");	//DHT并没有反馈
			return 0;
		}
		counter++;
		os_delay_us(1);
	}
	os_printf("高电平反馈持续时间:%d us\r\n",counter);
	//这时反馈信号已结束
	// 通过循环读取DHT返回的40位0、1数据

	for (i = 0; i < 5; i++)
	{
		for(j=0;j<8;j++)
		{
			counter = 0;
			//等待50us低电平过去（开始信号结束）
			while (GPIO_INPUT_GET(DHT_PIN) == 0)
			{
				counter++;
				os_delay_us(1);
				if (counter >= 1000)
				{
					os_printf("DHT11 Read Fail!");
					os_printf("起始位低电平超时!\r\n");	//DHT并没有反馈
					os_printf("起始位低电平持续时间:%d us\r\n",counter);
					return 0;
				}
			}
			os_printf("起始位低电平持续时间:%d us\r\n",counter);
			counter = 0;
			//0：高电平持续26-28us 1：高电平持续70us
			while (GPIO_INPUT_GET(DHT_PIN) == 1)
			{
				counter++;
				os_delay_us(1);
				if (counter >= 10000)
				{
					os_printf("DHT11 Read Fail!");
					os_printf("数据位高电平超时!\r\n");	//DHT并没有反馈
					os_printf("数据位位高电平持续时间:%d us\r\n",counter);
					return 0;
				}
			}

			os_printf("数据位高电平持续时间:%d us\r\n",counter);
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
	//退出禁止中断操作
	ets_intr_unlock();
	if(data[1]==0&&data[3]==0&&(data[0]+data[2]==data[4]))
	{
		os_printf("正确读取DHT11数据\r\n");
		*p_temperature=data[0];
		*p_humidity=data[2];
	}
	else
	{
		os_printf("DHT11数据读取错误\r\n");
		os_printf("data[0]:%x\t data[1]:%x\t data[2]:%x\t data[3]:%x\t data[4]:%x\r\n",data[0],data[1],data[2],data[3],data[4]);
	}
}


