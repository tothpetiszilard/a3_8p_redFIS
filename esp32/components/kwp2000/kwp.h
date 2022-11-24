#ifndef KWP2000_H_
#define KWP2000_H_

#include "stdint.h"

extern void Kwp_Init(uint8_t ecuId);

extern void Kwp_Receive(uint8_t * dataPtr,uint16_t len);
extern void Kwp_TxConfirmation(void);

#endif //KWP2000_H_

