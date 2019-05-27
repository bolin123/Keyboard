#include "System.h"

int main(void)
{
    SystemInitialize();
    while(1)
    {
        SystemPoll();
    }
}

