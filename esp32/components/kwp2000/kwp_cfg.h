/* Partial implementation of Keyword Protokoll */

#ifndef KWP2000_CFG_H_
#define KWP2000_CFG_H_

#include "vwtp.h"

#define KWP_TP_OK               (VWTP_OK)
#define KWP_TP_ERR              (VWTP_ERR)

#define KWP_TIMEOUT_1SEC        (20u)
#define KWP_TIMEOUT_5SECS       (100u)

#define Kwp_ConnectTp(ecuId)    (VwTp_Connect(ecuId))
#define Kwp_DisconnectTp()      (VwTp_Disconnect(1u))
#define Kwp_SendTp(msg)         (VwTp_Send(1u, msg, sizeof(msg)))

#endif

