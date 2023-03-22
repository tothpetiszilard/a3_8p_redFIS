#include "sysStates.h"
#include "stdio.h"

static uint8_t ignitionState_u8 = 0;

void SysStates_Receive(uint8_t data)
{
    ignitionState_u8 = (data & 0xFu);
}

uint8_t SysStates_GetIgnition(void)
{
    uint8_t retVal_u8 = 0;
    if (0 != (ignitionState_u8 & 7u))
    {
        retVal_u8 = 1;
    }
    else 
    {
        retVal_u8 = 0;
    }
    return retVal_u8;
}