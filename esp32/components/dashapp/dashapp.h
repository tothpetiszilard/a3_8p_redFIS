/* Emulation of Navigation which communicates to Dash */

#ifndef DASHAPP_H_
#define DASHAPP_H_

#include "stdint.h"

#define DASHAPP_OK    (0u)
#define DASHAPP_ERR   (1u)
#define DASHAPP_BUSY  (2u)
#define DASHAPP_PAUSE (3u)
typedef uint8_t DashApp_ReturnType;

typedef enum
{
    DASHAPP_FONT_L = 2u,
    DASHAPP_FONT_S = 6u,
} DashApp_FontType;

typedef enum
{
    DASHAPP_ADD = 0,
    DASHAPP_CLEAN
} DashApp_ModeType;

typedef struct
{
    uint8_t posX;
    uint8_t posY;
    uint8_t len;
    char string[14];
    DashApp_FontType ft;
    DashApp_ModeType mode;
}DashApp_ContentType;

extern void DashApp_Init(void);
extern void DashApp_Cyclic(void *pvParameters);

extern DashApp_ReturnType DashApp_GetStatus(void);
extern DashApp_ReturnType DashApp_Enter(void);
extern DashApp_ReturnType DashApp_Exit(void);
extern DashApp_ReturnType DashApp_Print(const DashApp_ContentType * const content);

extern uint8_t DashApp_Receive(uint8_t * dataPtr,uint16_t len);
extern void DashApp_TxConfirmation(uint8_t result);

#endif //DASHAPP_H_

