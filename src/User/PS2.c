#include "PS2.h"
#include "PS2Code.h"
#include "HalWait.h"
#include "VTStaticQueue.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"

#define PS2_CLK_PIN 0x0c
#define PS2_DIO_PIN 0x0b

typedef struct
{
    uint8_t ctrlkey;
    uint8_t reserve;
    uint8_t normalkey[6];
}PS2USBKey_t;

static VTSQueueDef(uint8_t, valueBuff, 128);
static PS2USBKey_t g_usbKey;

static void halExtiInit(bool interrupt)
{
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, GPIO_PinSource12);

    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line12; 
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; 
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; 
    EXTI_InitStructure.EXTI_LineCmd = ENABLE; 
    EXTI_Init(&EXTI_InitStructure);
    EXTI_ClearITPendingBit(EXTI_Line12);

    NVIC_InitTypeDef NVIC_InitStructure;
	
    /* Enable the USARTx Interrupt */ 
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = interrupt ? ENABLE : DISABLE; 
    NVIC_Init(&NVIC_InitStructure); 
    
}

static void ps2RecvModeSet(bool recvMode)
{
	halExtiInit(recvMode);
}

uint8_t check(uint8_t byte)
{
    uint8_t i, num = 0;

    for(i = 0; i < 8; i++)
    {
        if(byte & (0x01 << i))
        {
            num++;
        }
    }
    if(num % 2)
    {
        return 0;
    }
    return 1;
}

static void ps2BitRecv(uint8_t bit)
{
    static uint8_t bitcount = 0;
    static uint8_t recvByte = 0;
    static uint32_t lastTime;

    if(SysTimeHasPast(lastTime, 5))
		{
			bitcount = 0;
			recvByte = 0;
		}
		lastTime = SysTime();

    bitcount++;
    if(bitcount == 1)
    {
        if(bit != 0)
        {
            bitcount = 0;
        }
    }
    else if(bitcount == 10)
    {
        if(bit != check(recvByte))
        {
            bitcount = 0;
            recvByte = 0;
        }
    }
    else if(bitcount == 11)
    {
        if(bit == 1)
        {
            //input byte
            if(VTSQueueHasSpace(valueBuff))
            {
                VTSQueuePush(valueBuff, recvByte);
            }
            bitcount = 0;
            recvByte = 0;
        }
    }
    else
    {
        recvByte += (bit << (bitcount - 2));
    }
}

static void ps2BitSend(uint8_t byte)
{
	uint8_t bitcount = 9;
	uint32_t startTime = SysTime();
	
	ps2RecvModeSet(false);
	HalGPIOConfig(PS2_CLK_PIN, HAL_IO_OUTPUT);
	HalGPIOConfig(PS2_DIO_PIN, HAL_IO_OUTPUT);
	HalGPIOSetLevel(PS2_CLK_PIN, 0);
	HalWaitUs(200);
	HalGPIOSetLevel(PS2_DIO_PIN, 0);
	HalGPIOConfig(PS2_CLK_PIN, HAL_IO_INPUT);
	while(bitcount)
	{
		if(EXTI_GetITStatus(EXTI_Line12))
		{
			if(bitcount == 1)
			{
				HalGPIOSetLevel(PS2_DIO_PIN, 1);
			}
			else
			{
				HalGPIOSetLevel(PS2_DIO_PIN, (byte >> (9 - bitcount)) & 0x01);
			}
			
			EXTI_ClearITPendingBit(EXTI_Line12);
			bitcount--;
		}
	}
	HalWaitUs(15);
	HalGPIOConfig(PS2_DIO_PIN, HAL_IO_INPUT);
	ps2RecvModeSet(true);
}

static bool updateUsbKey(bool set, uint8_t value)
{
    uint8_t i;

    for(i = 0; i < 6; i++)
    {
        if(value == g_usbKey.normalkey[i])
        {
            if(set) //新增
            {
                return false; //已有，无需更新
            }
            else //删除
            {
                g_usbKey.normalkey[i] = 0;
                return true;
            }
        }
    }
    
    if(set)
    {
        for(i = 0; i < 6; i++)
        {
            if(g_usbKey.normalkey[i] == 0)
            {
                g_usbKey.normalkey[i] = value;
                return true;
            }
        }
    }

    return 0;
}

static void usbKeyHandle(bool ctrlKey, bool pushed, uint8_t value)
{
    bool needUpdate = false;
    if(value != 0xff)
    {
        //printf("U:%02x %d [%d]\n", value, pushed, ctrlKey);
        if(ctrlKey)
        {
            if(pushed)
            {
                if((g_usbKey.ctrlkey & (0x01 << value)) == 0)
                {
                    g_usbKey.ctrlkey |= (0x01 << value);
                    needUpdate = true;
                }
            }
            else
            {
                g_usbKey.ctrlkey &= ~(0x01 << value);
                needUpdate = true;
            }
        }
        else
        {
            if(pushed)
            {
                needUpdate = updateUsbKey(true, value);
            }
            else
            {
                needUpdate = updateUsbKey(false, value);
            }
        }

        if(needUpdate)
        {
            printf("\r\n");
            uint8_t *data = (uint8_t *)&g_usbKey;
            for(uint8_t i = 0; i < 8; i++)
            {
                printf("%02x ", data[i]);
            }
            printf("\r\n");
            // TODO: usb transfer function
        }
    }
}

static uint8_t ps2CtrlKey2UsbBitnum(uint16_t ps2Key)
{
    uint8_t i;

    for(i = 0; i < 8; i++)
    {
        if(ps2Key == g_CtrlKey[i])
        {
            return i;
        }
    }
    return 0xff;
}

static uint8_t ps2Key2Usbkey(uint16_t ps2Key)
{
    uint16_t i;

    for(i = 0; i < (sizeof(g_NormalKey) / 4); i++)
    {
        if(ps2Key == g_NormalKey[i][0])
        {
            return (uint8_t)g_NormalKey[i][1];
        }
    }

    return 0xff;
}

static void keyValueHandle(void)
{
    bool pushed = true;
    bool isCtrlKey = false;
	uint8_t value;
    uint16_t ps2key;
    uint8_t usbKey = 0;
    static bool gotE0 = false;
    static bool gotF0 = false;
	while(VTSQueueCount(valueBuff))
	{
		value = VTSQueueFront(valueBuff);
		VTSQueuePop(valueBuff);
		printf("%02x ", value);
        if(value == 0xE0)
        {
            gotE0 = true;
        }
        else if(value == 0xF0)
        {
            gotF0 = true;
        }
        else
        {

            ps2key = value;
            if(gotE0)
            {
               ps2key += 0xE000;
            }

            usbKey = ps2CtrlKey2UsbBitnum(ps2key);
            if(usbKey == 0xff)
            {
                isCtrlKey = false;
                usbKey = ps2Key2Usbkey(ps2key);
            }
            else
            {
                isCtrlKey = true;
            }

            if(gotF0)
            {
                pushed = false;
            }
            usbKeyHandle(isCtrlKey, pushed, usbKey);
            gotF0 = false;
            gotE0 = false;
        }
	}
}

void PS2SetLed(uint8_t value)
{
}

void PS2Reset(void)
{
	
}

void EXTI15_10_IRQHandler(void)
{
    //PS2BitRecv(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11));
    ps2BitRecv(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11));
    EXTI_ClearITPendingBit(EXTI_Line12);
}

void PS2Poll(void)
{
	keyValueHandle();
}

void PS2Init(void)
{
	HalGPIOConfig(PS2_CLK_PIN, HAL_IO_INPUT);
	HalGPIOConfig(PS2_DIO_PIN, HAL_IO_INPUT);
	ps2RecvModeSet(true);
	
	//ps2BitSend(0xff);
}

