#ifndef DASHAPP_H
#define DASHAPP_H

#include "stdint.h"

extern void DashApp_Init(void);

extern void DashApp_Receive(uint8_t * dataPtr,uint16_t len);
extern void DashApp_TxConfirmation(void);

#endif //DASHAPP_H

