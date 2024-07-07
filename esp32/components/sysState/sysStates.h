#ifndef SYSSTATES_H
#define SYSSTATES_H

#include "stdint.h"

#define SS_IGNITION_RXID   (0x575u)

extern uint8_t SysStates_GetIgnition(void);

extern void SysStates_Receive(uint8_t data);

#endif //SYSSTATES_H