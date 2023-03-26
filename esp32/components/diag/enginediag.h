/* Emulation of Tester which communicates to Engine controller */

#ifndef ENGINE_DIAG_H_
#define ENGINE_DIAG_H_

#include "stdint.h"
#include "diagcore.h"

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

extern Diag_ReturnType EngineDiag_Init(void);
#define EngineDiag_DeInit()    (Diag_DeInit())

extern Diag_ReturnType EngineDiag_GetChData(const EngineDiag_ChannelIdType ch, uint8_t * dataPtr, uint32_t timeout);

#endif //ENGINE_DIAG_H_
