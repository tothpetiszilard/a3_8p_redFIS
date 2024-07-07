/* Emulation of Dashboard which communicates to Navigation */

#ifndef NAVAPP_CFG_H
#define NAVAPP_CFG_H

#include "vwtp.h"
#include "sysStates.h"

#define NAVAPP_INIT_TIMEOUT            (120u)
#define NAVAPP_VWTP_CHID               (2u)
#define NAVAPP_DASHAPP_VWTP_CHID       (0u)

#define NAVAPP_CMD_PWRSTATE            ((uint8_t)0x00u)
#define NAVAPP_CMD_PWRREPORT           ((uint8_t)0x01u)
#define NAVAPP_CMD_IDREQ               ((uint8_t)0x08u)
#define NAVAPP_CMD_ID                  ((uint8_t)0x09u)
#define NAVAPP_CMD_ERR                 ((uint8_t)0x0Bu)
#define NAVAPP_CMD_PAGEREQ             ((uint8_t)0x20u)
#define NAVAPP_CMD_PAGERESP            ((uint8_t)0x21u)
#define NAVAPP_CMD_2E_REQ              ((uint8_t)0x2Eu)
#define NAVAPP_CMD_2F_RESP             ((uint8_t)0x2Fu)
#define NAVAPP_CMD_CLEAR               ((uint8_t)0x33u)
#define NAVAPP_CMD_SHOW                ((uint8_t)0x39u)
#define NAVAPP_CMD_REQAREA             ((uint8_t)0x52u)
#define NAVAPP_CMD_REQSTATUS           ((uint8_t)0x53u)
#define NAVAPP_CMD_WRITE               ((uint8_t)0x57u)

#define NAVAPP_SENDTP(buffer, len)             (VwTp_Send(NAVAPP_VWTP_CHID, buffer, len))
#define NAVAPP_DASHAPP_SENDTP(buffer, len)     (VwTp_Send(NAVAPP_DASHAPP_VWTP_CHID, buffer, len))
#define NAVAPP_DISCONNECT()                    (VwTp_Disconnect(NAVAPP_VWTP_CHID))
#define NAVAPP_GETIGNITION()                   (SysStates_GetIgnition())


#endif //NAVAPP_CFG_H

