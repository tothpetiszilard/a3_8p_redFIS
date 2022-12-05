#ifndef ENGINE_DIAG_H_
#define ENGINE_DIAG_H_

#include "stdint.h"

#define ENGINEDIAG_OK       (0u)
#define ENGINEDIAG_ERR      (1u)
#define ENGINEDIAG_PENDING  (2u)

typedef enum 
{
    ENGINEDIAG_CH_COOLANTTEMP1,
    ENGINEDIAG_CH_DURMI1,
    ENGINEDIAG_CH_QMI1,
    ENGINEDIAG_CH_ENGINESPEED1,
    ENGINEDIAG_CH_IATTEMP7,
    ENGINEDIAG_CH_FUELTEMP,
    ENGINEDIAG_CH_COOLANTTEMP7,
    ENGINEDIAG_CH_OILTEMP,
    ENGINEDIAG_CH_ENGINETEMP62,
    ENGINEDIAG_CH_COOLERTEMP62,
    ENGINEDIAG_CH_AMBITEMP62,
    ENGINEDIAG_CH_IATTEMP62,
    ENGINEDIAG_CH_EGTEMP67,
    ENGINEDIAG_CH_EGTEMP74,
    ENGINEDIAG_CH_LAMBDA74,
    ENGINEDIAG_CH_MAX,
}EngineDiag_ChannelIdType;

typedef uint8_t EngineDiag_ReturnType;


extern void EngineDiag_Init(void);
extern EngineDiag_ReturnType EngineDiag_GetChData(EngineDiag_ChannelIdType ch, uint8_t * dataPtr, uint32_t timeout);

#endif //ENGINE_DIAG_H_
