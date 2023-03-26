/* Emulation of Tester which communicates to Dashboard */

#ifndef DASH_DIAG_H_
#define DASH_DIAG_H_

#include "stdint.h"
#include "diagcore.h"

typedef enum 
{
    DASHDIAG_CH_VEHICLESPEED,
    DASHDIAG_CH_ENGINESPEED,
    DASHDIAG_CH_OILPRESSURE,
    DASHDIAG_CH_COOLANTTEMP,
    DASHDIAG_CH_FUELLEVEL1,
    DASHDIAG_CH_FUELLEVEL2,

    DASHDIAG_CH_MAX,
}DashDiag_ChannelIdType;

extern Diag_ReturnType DashDiag_Init(void);
#define DashDiag_DeInit()    (Diag_DeInit())
extern Diag_ReturnType DashDiag_GetChData(const DashDiag_ChannelIdType ch, uint8_t * dataPtr, uint32_t timeout);

#endif //DASH_DIAG_H_
