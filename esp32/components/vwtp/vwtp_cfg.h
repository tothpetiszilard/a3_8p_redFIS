#ifndef VWTP_20_CFG_H_
#define VWTP_20_CFG_H_

#include "stdint.h"
#include "canWrapper.h"

#define VWTP_TXBUFFERSIZE   (512)
#define VWTP_RXBUFFERSIZE   (512)

#define VWTP_SENDMESSAGE(id,len,dataPtr)      (Can_Write(id,len,dataPtr))

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

typedef struct 
{
    uint16_t rxId; // canID for reception
    uint16_t txId; // canID for sending frames
    uint8_t blockSize; // sent frames before ack
    uint8_t ackTimeout; // time until wait for ack
    uint8_t ips; // inter-packet-space, time between two TP frames
    VwTp_ModeType mode;
    void (*rxIndication)(uint8_t *data,uint16_t len); //callback: Data rx
    void (*txConfirmation)(void); //callback: Data sent
}VwTp_ChannelCfgType;

typedef struct
{
    uint8_t seqCntTx;
    uint8_t seqCntRx;
    uint8_t rxBuffer[VWTP_RXBUFFERSIZE];
    uint8_t txBuffer[VWTP_TXBUFFERSIZE];
    uint8_t txTimeout;
    uint16_t rxSize;
    uint16_t txSize;
    uint16_t txOffset;
    VwTp_StatesType txState;
    VwTp_StatesType rxState;
    VwTp_ChannelCfgType cfg;
}VwTp_ChannelType;

extern VwTp_ChannelType vwtp_channels[2];

#endif //VWTP_20_CFG_H_
