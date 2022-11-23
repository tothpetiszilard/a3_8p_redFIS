#ifndef CAN_WRAPPER_H
#define CAN_WRAPPER_H

#include "stdint.h"

#define CAN_OK      ((uint8_t)0)
#define CAN_ERR     ((uint8_t)1)

typedef uint8_t Can_ReturnType;

extern void Can_Init(void);
extern void Can_DeInit(void);
extern Can_ReturnType Can_Write(uint16_t id,uint8_t len,uint8_t * dataPtr);



#endif //CAN_WRAPPER_H