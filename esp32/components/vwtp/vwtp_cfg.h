#ifndef VWTP_20_CFG_H
#define VWTP_20_CFG_H

#include "stdint.h"
#include "canWrapper.h"

#define VWTP_TXBUFFERSIZE   (512)
#define VWTP_RXBUFFERSIZE   (512)

#define VWTP_SENDMESSAGE(id,len,dataPtr)      (Can_Write(id,len,dataPtr))

typedef enum
{
    VWTP_IDLE = 0u,
    VWTP_WAIT,
    VWTP_FINISHED
}VwTp_StatesType;

typedef struct 
{
    uint16_t rxId; // canID for reception
    uint16_t txId; // canID for sending frames
    uint8_t blockSize; // sent frames before ack
    uint8_t ackTimeout; // time until wait for ack
    uint8_t ips; // inter-packet-space, time between two TP frames
    void (*rxIndication)(uint8_t *data,uint8_t len); //callback: Data rx
    void (*txConfirmation)(void); //callback: Data sent
}VwTp_ChannelCfgType;

typedef struct
{
    uint8_t seqCntTx;
    uint8_t seqCntRx;
    uint8_t rxBuffer[VWTP_RXBUFFERSIZE];
    uint8_t txBuffer[VWTP_TXBUFFERSIZE];
    uint16_t rxSize;
    uint16_t txSize;
    uint16_t txOffset;
    VwTp_StatesType txState;
    VwTp_StatesType rxState;
    const VwTp_ChannelCfgType cfg;
}VwTp_ChannelType;

extern VwTp_ChannelType vwtp_channels[1];

#endif
