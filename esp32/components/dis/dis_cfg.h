#ifndef DIS_CFG_H_
#define DIS_CFG_H_

#include "stdint.h"
#include "dashapp.h"
#include "enginediag.h"

typedef struct
{
    const uint8_t rows_used;
    const DashApp_ContentType * const labels;
    const DashApp_ContentType * const data;
    const EngineDiag_ChannelIdType * const diagChs;
}DisPageType;

typedef struct
{
    uint8_t didBuffer[12u];
    uint8_t dataId;
} DiagIdType;

extern const DashApp_ContentType pageLabels[1][3];
extern const DashApp_ContentType pageData[1][3];
extern const EngineDiag_ChannelIdType pageDiag[1][3];

extern uint8_t Dis_DecodeFrame(char *p, uint8_t *frameData);

#endif //DIS_CFG_H_

