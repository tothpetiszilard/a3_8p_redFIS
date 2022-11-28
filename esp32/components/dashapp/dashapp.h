#ifndef DASHAPP_H
#define DASHAPP_H

#include "stdint.h"

#define DASHAPP_OK   (0u)
#define DASHAPP_ERR  (1u)
#define DASHAPP_BUSY (2u)
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
    char string[10];
    DashApp_FontType ft;
    DashApp_ModeType mode;
}DashApp_ContentType;

extern void DashApp_Init(void);

extern DashApp_ReturnType DashApp_Print(DashApp_ContentType * content);

extern void DashApp_Receive(uint8_t * dataPtr,uint16_t len);
extern void DashApp_TxConfirmation(void);

#endif //DASHAPP_H

