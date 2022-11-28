#ifndef VWTP_20_H_
#define VWTP_20_H_

#include "stdint.h"

#define VWTP_OK         ((uint8_t)0u)
#define VWTP_ERR        ((uint8_t)1u)
#define VWTP_PENDING    ((uint8_t)2u)

typedef uint8_t VwTp_ReturnType;

extern void VwTp_Init(void);
extern VwTp_ReturnType VwTp_Connect(uint8_t ecuId);
extern void VwTp_Disconnect(uint8_t chId);
extern VwTp_ReturnType VwTp_Send(uint8_t chId, uint8_t * buffer, uint16_t len);
extern void VwTp_Receive(uint16_t canId, uint8_t dlc, uint8_t * dataPtr);


#endif //VWTP_20_H_

