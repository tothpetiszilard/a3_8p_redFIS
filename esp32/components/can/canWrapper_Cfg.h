#ifndef CAN_WRAPPER_CFG_H
#define CAN_WRAPPER_CFG_H

#include "vwtp.h"

#define CAN_RXINDICATION(canId, dlc, dataPtr)      (VwTp_Receive(canId, dlc, dataPtr))

#endif //CAN_WRAPPER_CFG_H