#include "Temperature.h"
#include "HalWait.h"

#define DQ_GPIO_NUM 0x30

#define DQ_DIRCT_OUTPUT() HalGPIOConfig(DQ_GPIO_NUM, HAL_IO_OUTPUT)
#define DQ_DIRCT_INPUT() HalGPIOConfig(DQ_GPIO_NUM, HAL_IO_INPUT)
#define DQ_SET_LEVEL(x) HalGPIOSetLevel(DQ_GPIO_NUM, x)
#define DQ_GET_LEVEL() HalGPIOGetLevel(DQ_GPIO_NUM)
 
/**************************************
复位DS18B20,并检测设备是否存在
**************************************/
static int DS18B20Reset(void)
{
	uint8_t i;
	
	for(i = 0; i < 10; i++)
	{
		DQ_DIRCT_OUTPUT();
		DQ_SET_LEVEL(0);
		HalWaitUs(650);
		DQ_SET_LEVEL(1);
		HalWaitUs(50);
		DQ_DIRCT_INPUT();
		HalWaitUs(650);
		if(DQ_GET_LEVEL() == 0)
		{
			return 0;
		}
	}
	return -1;
}
 
/**************************************
从DS18B20读1字节数据
**************************************/
static uint8_t DS18B20ReadByte(void)
{
    uint8_t i;
    uint8_t dat = 0;
 
    for (i=0; i<8; i++)             //8位计数器
    {
    	DQ_DIRCT_OUTPUT();
        dat >>= 1;
        DQ_SET_LEVEL(0);                     //开始时间片
        HalWaitUs(5);                //延时等待
        DQ_SET_LEVEL(1);                     //准备接收
        HalWaitUs(5);                //接收延时
        DQ_DIRCT_INPUT();
		HalWaitUs(5);
        if (DQ_GET_LEVEL()) 
		{
			dat |= 0x80;        //读取数据
		}
        HalWaitUs(60);               //等待时间片结束
    }
 
    return dat;
}
 
/**************************************
向DS18B20写1字节数据
**************************************/
static void DS18B20WriteByte(uint8_t dat)
{
    char i;
	uint8_t value = dat;
 
	DQ_DIRCT_OUTPUT();
    for (i=0; i<8; i++)             //8位计数器
    {
        DQ_SET_LEVEL(0);                     //开始时间片
        HalWaitUs(5);                //延时等待
        DQ_SET_LEVEL(value & 0x01);
		value = value >> 1;
        HalWaitUs(60);               //等待时间片结束
        DQ_SET_LEVEL(1);                     //恢复数据线
        HalWaitUs(5);                //恢复延时
    }
}

uint16_t TemperatureGetValue(void)
{
	uint8_t tpl;	
	uint16_t value;
	DS18B20Reset();				//设备复位
	DS18B20WriteByte(0xCC);		//跳过ROM命令
	DS18B20WriteByte(0x44);		//开始转换命令
	
	DQ_DIRCT_INPUT();
	while (!DQ_GET_LEVEL());			    //等待转换完成
 
	DS18B20Reset();				//设备复位
	DS18B20WriteByte(0xCC);		//跳过ROM命令
	DS18B20WriteByte(0xBE);		//读暂存存储器命令
	tpl = DS18B20ReadByte();		//读温度低字节
	value = DS18B20ReadByte();		//读温度高字节
	return ((value << 8) + tpl);
}


void TemperatureInitialize(void)
{

}

void TemperaturePoll(void)
{
}

