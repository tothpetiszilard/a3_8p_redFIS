#ifndef CAN_WRAPPER_CFG_H
#define CAN_WRAPPER_CFG_H

#include "vwtp.h"
#include "stalkButtons.h"
#include "sysStates.h"

#define CAN_STALK_RXID                               (STALKBUTTONS_RXID)
#define CAN_IGNITION_RXID                            (SS_IGNITION_RXID)

#define CANTP_RXINDICATION(canId, dlc, dataPtr)      (VwTp_Receive(canId, dlc, dataPtr))
#define CAN_STALK_RXINDICATION(data)                 (StalkButtons_Receive(data))
#define CAN_IGN_RXINDICATION(data)                   (SysStates_Receive(data))

#endif //CAN_WRAPPER_CFG_H