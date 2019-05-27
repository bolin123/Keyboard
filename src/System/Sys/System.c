#include "System.h"
#include "PS2.h"

void SystemReboot(void)
{
	HalReboot();
}

void SystemInitialize(void)
{
	HalInit();
	PS2Init();
}

void SystemPoll(void)
{
	HalPoll();
	PS2Poll();
}
