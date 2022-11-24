#ifndef STALKBUTTONS_H
#define STALKBUTTONS_H

#include "stdint.h"

#define STALKBUTTONS_RXID   (0x35Fu)

typedef enum
{
    STALKBUTTONS_NOEVENT,
    STALKBUTTONS_UP = 1u,
    STALKBUTTONS_DOWN = 2u,
    STALKBUTTONS_RESET = 4u
}StalkButtons_Type;

extern StalkButtons_Type StalkButtons_Get(void);

extern void StalkButtons_Receive(uint8_t buttons);

#endif //STALKBUTTONS_H