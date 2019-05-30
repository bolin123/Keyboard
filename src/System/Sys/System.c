#include "System.h"
#include "PS2.h"
#include "Wifi.h"
#include "MSocket.h"
#include "Demo.h"

void SystemReboot(void)
{
	HalReboot();
}

void SystemInitialize(void)
{
	HalInit();
	PS2Init();
	if(WifiInit() < 0)
	{
		SysLog("Wifi init error!");
		HalReboot();
		while(1);
	}
	MSocketInit();
	DemoInit();
}

void SystemPoll(void)
{
	HalPoll();
	PS2Poll();
	WifiPoll();
	MSocketPoll();
	DemoPoll();
}
