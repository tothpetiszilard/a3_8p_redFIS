#ifndef CAN_WRAPPER_CFG_H
#define CAN_WRAPPER_CFG_H

#include "vwtp.h"
#include "stalkButtons.h"

#define CAN_RXID                                     (STALKBUTTONS_RXID)

#define CANTP_RXINDICATION(canId, dlc, dataPtr)      (VwTp_Receive(canId, dlc, dataPtr))
#define CAN_RXINDICATION(data)                       (StalkButtons_Receive(data))

#endif //CAN_WRAPPER_CFG_H