#ifndef PS2_H
#define PS2_H

#include "System.h"

typedef enum
{
	PS2_LED_ScrollLock = 0,
	PS2_LED_NumLock,
	PS2_LED_CapsLock,
}PS2LedId_t;

void PS2Init(void);
void PS2Poll(void);

#endif

