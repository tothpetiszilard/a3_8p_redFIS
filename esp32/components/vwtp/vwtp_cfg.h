#ifndef VWTP_20_CFG_H_
#define VWTP_20_CFG_H_

#include <stdint.h>
#include <stddef.h>
#include "canWrapper.h"
#include "sdkconfig.h"

#define VWTP_TXBUFFERSIZE        (256)
#define VWTP_RXBUFFERSIZE        (256)

#define VWTP_SENDMESSAGE(id,len,dataPtr)      (Can_Write(id,len,dataPtr))

#define VWTP_TPPARAMS_MASK       (1u)
#define VWTP_TPPARAMS_REQUEST    (2u)
#define VWTP_TPPARAMS_RESPONSE   (3u)

#define VWTP_TXTASK_ACK_READY    (1u)
#define VWTP_TXTASK_ACK_NOTREADY (2u)

typedef enum
{
    VWTP_CONNECT = 0u,
    VWTP_IDLE,
    VWTP_WAIT,
    VWTP_ACK,
    VWTP_FINISHED
}VwTp_StatesType;

typedef enum
{
    VWTP_DIAG = 0u,
    VWTP_NONDIAG
}VwTp_ModeType;

typedef enum
{
    VWTP_APP_READY = 0u,
    VWTP_APP_BUSY
}VwTp_AppState;

typedef struct
{
    uint8_t ack : 2; // 1 = ack, ready 2 = ack, not ready
    uint8_t params :2; // 2 = req, 3 = resp
    uint8_t brk :1;
} VwTp_TxTasks;

typedef struct 
{
    uint16_t rxId; // canID for reception
    uint16_t txId; // canID for sending frames
    uint8_t blockSize; // sent frames before ack
    uint8_t ackTimeout; // time until wait for ack
    uint8_t ips; // inter-packet-space, time between two TP frames
    VwTp_ModeType mode;
    #if (0 != CONFIG_VWTP_NAV_ROUTING)
    uint8_t (*appStatus)(void); //callback: Get application state (ready or not)
    #endif
    void (*rxIndication)(uint8_t *data,uint16_t len); //callback: Data rx
    void (*txConfirmation)(uint8_t result); //callback: Data sent
}VwTp_ChannelCfgType;

typedef struct
{
    uint8_t seqCntTx;
    uint8_t ackSeqCntTx; // last acknowledged tx seq cnt
    uint8_t seqCntRx;
    uint8_t ackSeqCntRx; // last acknowledged rx seq cnt
    uint8_t rxBuffer[VWTP_RXBUFFERSIZE];
    uint8_t txBuffer[VWTP_TXBUFFERSIZE];
    uint8_t txTimeout;
    uint8_t rxTimeout;
    uint16_t rxSize;
    uint16_t txSize;
    uint16_t txOffset;
    VwTp_StatesType txState;
    VwTp_StatesType rxState;
    VwTp_TxTasks txFlags;
    VwTp_ChannelCfgType cfg;
}VwTp_ChannelType;

#if ((1 == CONFIG_VWTP_DASH_TX_ID_ALTERNATIVE) && ( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE))
    #error "RNS-E and Alternative can not be selected in the same time!"
#elif (1 == CONFIG_VWTP_DASH_TX_ID_ALTERNATIVE)
extern VwTp_ChannelType vwtp_channels[2];
#elif( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE)
    #if (0 != CONFIG_VWTP_NAV_ROUTING)
extern VwTp_ChannelType vwtp_channels[3];
    #else
extern VwTp_ChannelType vwtp_channels[2];
    #endif //CONFIG_VWTP_NAV_ROUTING
#endif

#endif //VWTP_20_CFG_H_
