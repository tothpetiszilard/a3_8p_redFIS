#ifndef DIS_CFG_H_
#define DIS_CFG_H_

#include "stdint.h"
#include "dashapp.h"
#include "navapp.h"
#include "enginediag.h"
#include "dashdiag.h"
#include "sdkconfig.h"

typedef struct
{
    EngineDiag_ChannelIdType diagCh;
    uint16_t timeout;
} DiagIdType;

typedef struct
{
    const uint8_t labelCnt;
    const DashApp_ContentType * const labels;
    const uint8_t dataCnt;
    const DashApp_ContentType * const data;
    const uint8_t diagCnt;
    const DiagIdType * const diagChs;
}DisPageType;

extern const DashApp_ContentType pageLabels[1][3];
extern const DashApp_ContentType pageData[1][3];
extern const DiagIdType pageDiag[1][3];

extern uint8_t Dis_DecodeFrame(char *p, uint8_t *frameData);

#endif //DIS_CFG_H_

