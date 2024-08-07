/* Emulation of Navigation which communicates to Dash */

#ifndef DASHAPP_CFG_H
#define DASHAPP_CFG_H

#include "vwtp.h"
#include "sysStates.h"

#define DASHAPP_CMD_PWRSTATE            ((uint8_t)0x00u)
#define DASHAPP_CMD_PWRREPORT           ((uint8_t)0x01u)
#define DASHAPP_CMD_IDREQ               ((uint8_t)0x08u)
#define DASHAPP_CMD_ID                  ((uint8_t)0x09u)
#define DASHAPP_CMD_ERR                 ((uint8_t)0x0Bu)
#define DASHAPP_CMD_PAGEREQ             ((uint8_t)0x20u)
#define DASHAPP_CMD_PAGERESP            ((uint8_t)0x21u)
#define DASHAPP_CMD_2E_REQ              ((uint8_t)0x2Eu)
#define DASHAPP_CMD_2F_RESP             ((uint8_t)0x2Fu)
#define DASHAPP_CMD_CLEAR               ((uint8_t)0x33u)
#define DASHAPP_CMD_SHOW                ((uint8_t)0x39u)
#define DASHAPP_CMD_REQAREA             ((uint8_t)0x52u)
#define DASHAPP_CMD_REQSTATUS           ((uint8_t)0x53u)
#define DASHAPP_CMD_WRITE               ((uint8_t)0x57u)

#define DASHA_VWTP_CHID                 (0u)

#define DASHAPP_SENDTP(buffer, len)     (VwTp_Send(DASHA_VWTP_CHID, buffer, len))
#define DASHAPP_READYCALLBACK()         (VwTp_RxReady_Cb(DASHA_VWTP_CHID))
#define DASHAPP_DISCONNECT()            (VwTp_Disconnect(DASHA_VWTP_CHID))
#define DASHAPP_GETIGNITION()           (SysStates_GetIgnition())

const uint8_t DashApp_DspInit[] = 
{
    0x52,
    0x05,
    0x02,
    0x00,
    0x1B,
    0x40,
    0x30,
    0x57,
    0x0E,
    0x02,
    0x00,
    0x15,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x57,
    0x0E,
    0x02,
    0x00,
    0x1F,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x57,
    0x0E,
    0x02,
    0x00,
    0x29,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x57,
    0x0D,
    0x26,
    0x00,
    0x05,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65,
    0x65
};

#endif //DASHAPP_CFG_H

