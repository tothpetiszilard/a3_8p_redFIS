/* Emulation of Dashboard which communicates to Navigation */

#ifndef NAVAPP_H_
#define NAVAPP_H_

#include "stdint.h"

#define NAVAPP_OK    (0u)
#define NAVAPP_ERR   (1u)
#define NAVAPP_BUSY  (2u)
#define NAVAPP_PAUSE (3u)
typedef uint8_t NavApp_ReturnType;

extern void NavApp_Init(void);
extern void NavApp_Cyclic(void *pvParameters);

extern NavApp_ReturnType NavApp_GetStatus(void);
extern NavApp_ReturnType NavApp_Pause(void);
extern NavApp_ReturnType NavApp_Continue(void);
extern NavApp_ReturnType NavApp_GetRxStatus(void);

extern void NavApp_Receive(uint8_t * dataPtr,uint16_t len);
extern void NavApp_TxConfirmation(uint8_t result);

#endif //NAVAPP_H_

