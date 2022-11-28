#ifndef KWP2000_H_
#define KWP2000_H_

#include "stdint.h"

#define KWP_OK  (0u)
#define KWP_ERR (1u)

typedef uint8_t Kwp_ReturnType;

extern void Kwp_Init(uint8_t ecuId);

extern Kwp_ReturnType Kwp_GetDataFromECU(uint8_t * dataPtr);

extern void Kwp_Receive(uint8_t * dataPtr,uint16_t len);
extern void Kwp_TxConfirmation(void);

#endif //KWP2000_H_

