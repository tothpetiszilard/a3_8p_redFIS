#include "vwtp.h"
#include "vwtp_cfg.h"
#include "stddef.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#define VWTP_PARAMS_RESPONSE    (1u)
#define VWTP_PARAMS_REQUEST     (0u)

static TaskHandle_t VwTpTaskHdl = NULL;

static void VwTp_HandleTx(VwTp_ChannelType * const chPtr);
static void VwTp_HandleRx(VwTp_ChannelType * chPtr,uint8_t dlc,uint8_t * dataPtr);
static void VwTp_HandleConnect(VwTp_ChannelType * chPtr,uint8_t * dataPtr);
static void VwTp_HandleCallbacks(VwTp_ChannelType * const chPtr);
static void VwTp_sendTpParams(VwTp_ChannelType * const chPtr, uint8_t response);
static void VwTp_sendClose(VwTp_ChannelType * const chPtr);
static void VwTp_sendAck(VwTp_ChannelType * const chPtr);

static void VwTp_Cyclic(void *pvParameters);

void VwTp_Init(void)
{
    uint8_t i = 0;
    for (i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        if (VWTP_DIAG == vwtp_channels[i].cfg.mode)
        {
            vwtp_channels[i].txState = VWTP_CONNECT;
            vwtp_channels[i].rxState = VWTP_CONNECT;
        }
        else 
        {
            vwtp_channels[i].txState = VWTP_IDLE;
            vwtp_channels[i].rxState = VWTP_IDLE;
        }
    }
    xTaskCreatePinnedToCore(VwTp_Cyclic, "VwTp", 2048, NULL, 5, &VwTpTaskHdl,1);
}

VwTp_ReturnType VwTp_Connect(uint8_t ecuId)
{
    uint8_t i = 0;
    uint8_t msg[7];
    VwTp_ChannelType * chPtr= NULL;
    VwTp_ReturnType retVal = VWTP_ERR;
    for (i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        if (0x200u == vwtp_channels[i].cfg.txId)
        {
            chPtr = &vwtp_channels[i];
            break;
        }
    }
    if (NULL != chPtr)
    {
        if (chPtr->txState == VWTP_CONNECT)
        {
            msg[0] = ecuId;
            msg[1] = 0xC0u; // connect
            msg[2] = 0x00u; // rxId (lsb)
            msg[3] = 0x10u; // invalid rxId (msb)
            msg[4] = 0x00u; // txId (lsb)
            msg[5] = 0x03u; // valid txId (msb) 0x300
            msg[6] = 0x01u; // app = kwp2000
            if (CAN_OK == VWTP_SENDMESSAGE(chPtr->cfg.txId,sizeof(msg),msg))
            {
                retVal = VWTP_OK;
                chPtr->cfg.rxId = (0x200u | ecuId); // Preparation for response
            }
        }
    }
    return retVal;
}

static void VwTp_Cyclic(void *pvParameters)
{
    uint8_t chId = 0;
    VwTp_ChannelType * chPtr = NULL;
    while(1)
    {
        for (chId = 0;chId < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));chId++ )
        {
            chPtr = &vwtp_channels[chId];
            VwTp_HandleTx(chPtr); // TODO: Requesting ACK after x transmitted frames 
            VwTp_HandleCallbacks(chPtr);
            //TODO: Timout handling is not implemented
            
        }
        vTaskDelay(10 * portTICK_PERIOD_MS);
    }
    
}

VwTp_ReturnType VwTp_Send(uint8_t chId, uint8_t * buffer, uint16_t len)
{
    VwTp_ChannelType * chPtr = NULL;
    VwTp_ReturnType retVal = VWTP_ERR;
    uint8_t i = 0;
    if (chId < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0])))
    {
        chPtr = &vwtp_channels[chId];
        if (chPtr->txState == VWTP_IDLE)
        {
            vTaskSuspendAll(); // Critical section, interrupts enabled
            chPtr->txSize = len;
            chPtr->txOffset = 0u;
            for (i=0; i<len; i++)
            {
                chPtr->txBuffer[i] = buffer[i];
            }
            chPtr->txState = VWTP_WAIT;
            xTaskResumeAll(); // End of critical section, interrupts enabled
            retVal = VWTP_OK;
        }
    }
    return retVal;
}

void VwTp_Receive(uint16_t canId, uint8_t dlc, uint8_t * dataPtr)
{
    uint8_t i = 0;
    VwTp_ChannelType * chPtr= NULL;
    for (i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        if (canId == vwtp_channels[i].cfg.rxId)
        {
            chPtr = &vwtp_channels[i];
            break;
        }
    }
    if (NULL != chPtr)
    {
        if (chPtr->rxState != VWTP_CONNECT)
        {
            VwTp_HandleRx(chPtr,dlc,dataPtr);
        }
        else 
        {
            VwTp_HandleConnect(chPtr,dataPtr);
        }
    }
}

void VwTp_Disconnect(uint8_t chId)
{
    VwTp_ChannelType * chPtr = NULL;
    if (chId < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0])))
    {
        chPtr = &vwtp_channels[chId];
        vTaskSuspendAll(); // Critical section, interrupts enabled
        chPtr->seqCntTx = 0;
        chPtr->seqCntRx = 0;
        chPtr->rxSize = 0;
        chPtr->txSize = 0;
        chPtr->rxState = VWTP_IDLE;
        chPtr->txState = VWTP_IDLE;
        xTaskResumeAll(); // End of critical section, interrupts enabled
        VwTp_sendClose(chPtr);
        // Reset diagnostics
        if (VWTP_DIAG == chPtr->cfg.mode)
        {
            vTaskSuspendAll(); // Critical section, interrupts enabled
            chPtr->rxState = VWTP_CONNECT;
            chPtr->txState = VWTP_CONNECT;
            chPtr->cfg.txId = 0x200u;
            chPtr->cfg.rxId = 0;
            xTaskResumeAll(); // End of critical section, interrupts enabled
        }
    }
}

static void VwTp_HandleConnect(VwTp_ChannelType * chPtr,uint8_t * dataPtr)
{
    // Connection response
    if ((0x00u == dataPtr[0]) && (0xD0u == dataPtr[1]) && (0u == (dataPtr[3] & 0x10u)) && (0u == (dataPtr[5] & 0x10u)))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        chPtr->cfg.rxId = (dataPtr[2] | (dataPtr[3]<<8u));
        chPtr->cfg.txId = (dataPtr[4] | (dataPtr[5]<<8u));
        chPtr->rxState = VWTP_IDLE;
        chPtr->txState = VWTP_IDLE;
        xTaskResumeAll(); // End of critical section, interrupts enabled
        VwTp_sendTpParams(chPtr,VWTP_PARAMS_REQUEST);
    }
}

static void VwTp_HandleRx(VwTp_ChannelType * chPtr,uint8_t dlc,uint8_t * dataPtr)
{
    uint8_t i = 0;
    if ((0xA3u == dataPtr[0]) || (0xA0u == dataPtr[0]))
    {
        // Connection init or alive check
        VwTp_sendTpParams(chPtr,VWTP_PARAMS_RESPONSE);
    }
    else if (0xA1u == dataPtr[0])
    {
        // Connection initialized or response to alive check
        chPtr->txState = VWTP_FINISHED; // notify app
    }
    else if (0x10u == (dataPtr[0] & 0xF0u))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        chPtr->seqCntRx = (dataPtr[0] & 0xF);
        //last frame or single frame
        if (chPtr->rxState == VWTP_IDLE)
        {
            //single frame received
            chPtr->rxSize = (dlc-1);
            for(i=0;i<chPtr->rxSize;i++)
            {
                chPtr->rxBuffer[i] = dataPtr[i+1];
            }
        }
        else
        {
            // last frame received
            for(i=0;i<(dlc-1);i++)
            {
                chPtr->rxBuffer[chPtr->rxSize + i] = dataPtr[i+1];
            }
            chPtr->rxSize += (dlc-1);
        }
        xTaskResumeAll(); // End of critical section, interrupts enabled
        VwTp_sendAck(chPtr); // Ack + rxIndication
    }
    else if ((0x20u == (dataPtr[0] & 0xF0u)) || (0u == (dataPtr[0] & 0xF0u)))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        chPtr->seqCntRx = (dataPtr[0] & 0x0Fu);
        chPtr->rxState = VWTP_WAIT; // more data expected
        for(i=0;i<(dlc-1);i++)
        {
            chPtr->rxBuffer[chPtr->rxSize + i] = dataPtr[i+1];
        }
        chPtr->rxSize += (dlc-1);
        xTaskResumeAll(); // End of critical section, interrupts enabled
        if (0 == (dataPtr[0] & 0xF0u))
        {
            VwTp_sendAck(chPtr);
        }
    }
    else if (0xB0u == (dataPtr[0] & 0xF0u))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        chPtr->seqCntTx = (dataPtr[0] & 0x0Fu);
        chPtr->txSize = 0;
        chPtr->txOffset = 0;
        chPtr->txState = VWTP_FINISHED; // txConfirmation
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    else if (0xA8u == dataPtr[0])
    {
        // Connection was terminated
        vTaskSuspendAll(); // Critical section, interrupts enabled
        chPtr->seqCntTx = 0;
        chPtr->seqCntRx = 0;
        chPtr->rxSize = 0;
        chPtr->txSize = 0;
        chPtr->rxState = VWTP_IDLE;
        chPtr->txState = VWTP_IDLE;
        VwTp_sendClose(chPtr);
        // Reset diagnostics
        if (chPtr->cfg.mode == VWTP_DIAG)
        {
            chPtr->rxState = VWTP_CONNECT;
            chPtr->txState = VWTP_CONNECT;
            chPtr->cfg.txId = 0x200u;
            chPtr->cfg.rxId = 0;
        }
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    else
    {
        // error case
        VwTp_sendClose(chPtr);
    }
}

static void VwTp_sendAck(VwTp_ChannelType * const chPtr)
{
    uint8_t msg[1];
    uint8_t ackSeq;
    if (chPtr->txState == VWTP_IDLE)
    {
        if (chPtr->seqCntRx < 0x0Fu)
        {
            ackSeq = chPtr->seqCntRx+1;
        }
        else
        {
            ackSeq = 0;
        }
        msg[0] = 0xB0u | ackSeq; // ack
        VWTP_SENDMESSAGE(chPtr->cfg.txId,sizeof(msg),msg);
        // reset states to receive the next msg
        chPtr->rxState = VWTP_FINISHED;
    }
}

static void VwTp_sendClose(VwTp_ChannelType * const chPtr)
{
    uint8_t tpClose[1] = {0xA8u};
    if (chPtr->txState == VWTP_IDLE)
    {
        VWTP_SENDMESSAGE(chPtr->cfg.txId,sizeof(tpClose),tpClose);
    }
}

static void VwTp_sendTpParams(VwTp_ChannelType * const chPtr, uint8_t response)
{
    uint8_t tpParams[6];
    if ((chPtr->txState == VWTP_IDLE) && (response < 2))
    {
        tpParams[0u] = 0xA0u | response; // params resp
        tpParams[1u] = chPtr->cfg.blockSize; // msg num before ack
        tpParams[2u] = chPtr->cfg.ackTimeout; // timing
        tpParams[3u] = 0xFFu; // dummy
        tpParams[4u] = chPtr->cfg.ips; // timing2
        tpParams[5u] = 0xFFu; // dummy
        VWTP_SENDMESSAGE(chPtr->cfg.txId,sizeof(tpParams),tpParams);
    }
}

static void VwTp_HandleCallbacks(VwTp_ChannelType * const chPtr)
{
    
    if (chPtr->rxState == VWTP_FINISHED)
    {
        if (NULL != chPtr->cfg.rxIndication)
        {
            chPtr->cfg.rxIndication(chPtr->rxBuffer,chPtr->rxSize);
        }
        chPtr->rxSize = 0u;
        chPtr->rxState = VWTP_IDLE;
    }
    if (chPtr->txState == VWTP_FINISHED)
    {
        if (NULL != chPtr->cfg.txConfirmation)
        {
            chPtr->cfg.txConfirmation();
        }
        chPtr->txState = VWTP_IDLE;
    }
}

static void VwTp_HandleTx(VwTp_ChannelType * const chPtr)
{
    uint16_t tmp;
    uint8_t dlc;
    uint8_t msg[8];
    
    if (chPtr->txState == VWTP_WAIT)
    {
        if ((chPtr->txSize < 8u) || (((chPtr->txSize)-(chPtr->txOffset)) < 8u))
        {
            dlc = ((chPtr->txSize)-(chPtr->txOffset))+1u;
            if (dlc != 1)
            {
                msg[0] = 0x10u | chPtr->seqCntTx; // single frame or last frame
                for (tmp=0;tmp<(dlc-1);tmp++)
                {
                    msg[tmp+1] = chPtr->txBuffer[tmp+(chPtr->txOffset)];
                }
                if (CAN_OK == VWTP_SENDMESSAGE(chPtr->cfg.txId,dlc,msg))
                {
                    chPtr->txState = VWTP_ACK;
                }
            }
            else
            {
                msg[0] = 0xA3; // alive check
                if (CAN_OK == VWTP_SENDMESSAGE(chPtr->cfg.txId,dlc,msg))
                {
                    chPtr->txState = VWTP_ACK;
                }
            }
        }
        else
        {
            dlc = 8;
            msg[0] = 0x20u | chPtr->seqCntTx; // consecutive frame
            for (tmp=0;tmp<7u;tmp++)
            {
                msg[tmp+1u] = chPtr->txBuffer[tmp+(chPtr->txOffset)];
            }
            if (CAN_OK == VWTP_SENDMESSAGE(chPtr->cfg.txId,dlc,msg))
            {
                chPtr->txOffset += 7u;
                if (chPtr->seqCntTx < 0xFu)
                {
                    chPtr->seqCntTx++;
                }
                else
                {
                    chPtr->seqCntTx = 0u;
                }
            }
        }
    }
}

