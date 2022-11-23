#include "vwtp.h"
#include "vwtp_cfg.h"
#include "stddef.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t VwTpTaskHdl = NULL;

static void VwTp_HandleTx(VwTp_ChannelType * const chPtr);
static void VwTp_HandleCallbacks(VwTp_ChannelType * const chPtr);
static void VwTp_sendTpParams(VwTp_ChannelType * const chPtr);
static void VwTp_sendClose(VwTp_ChannelType * const chPtr);
static void VwTp_sendAck(VwTp_ChannelType * const chPtr);

static void VwTp_Cyclic(void *pvParameters);

void VwTp_Init(void)
{
    xTaskCreate(VwTp_Cyclic, "VwTp", 2048, NULL, 5, &VwTpTaskHdl);
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
        if ((0xA3u == dataPtr[0]) || (0xA0u == dataPtr[0]))
        {
            // Connection init or alive check
            VwTp_sendTpParams(chPtr);
        }
        else if (0x10u == (dataPtr[0] & 0xF0u))
        {
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
                VwTp_sendAck(chPtr); // Ack + rxIndication
            }
            else
            {
                // last frame received
                for(i=0;i<(dlc-1);i++)
                {
                    chPtr->rxBuffer[chPtr->rxSize + i] = dataPtr[i+1];
                }
                chPtr->rxSize += (dlc-1);
                VwTp_sendAck(chPtr); // Ack + rxIndication
            }
        }
        else if ((0x20u == (dataPtr[0] & 0xF0u)) || (0u == (dataPtr[0] & 0xF0u)))
        {
            chPtr->seqCntRx = (dataPtr[0] & 0x0Fu);
            chPtr->rxState = VWTP_WAIT; // more data expected
            for(i=0;i<(dlc-1);i++)
            {
                chPtr->rxBuffer[chPtr->rxSize + i] = dataPtr[i+1];
            }
            chPtr->rxSize += (dlc-1);
            if (0 == (dataPtr[0] & 0xF0u))
            {
                VwTp_sendAck(chPtr);
            }
        }
        else if (0xB0u == (dataPtr[0] & 0xF0u))
        {
            chPtr->seqCntTx = (dataPtr[0] & 0x0Fu);
            chPtr->txSize = 0;
            chPtr->txOffset = 0;
            chPtr->txState = VWTP_FINISHED; // txConfirmation
        }
        else if (0xA8u == dataPtr[0])
        {
            chPtr->seqCntTx = 0;
            chPtr->seqCntRx = 0;
            chPtr->rxSize = 0;
            chPtr->txSize = 0;
            chPtr->rxState = VWTP_IDLE;
            chPtr->txState = VWTP_IDLE;
            // Connection was terminated
            VwTp_sendClose(chPtr);
        }
        else
        {
            // error case
            VwTp_sendClose(chPtr);
        }
    }
}

static void VwTp_sendAck(VwTp_ChannelType * const chPtr)
{
    if (chPtr->txState == VWTP_IDLE)
    {
        uint8_t msg[1];
        uint8_t ackSeq;
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

static void VwTp_sendTpParams(VwTp_ChannelType * const chPtr)
{
    uint8_t tpParams[6];
    if (chPtr->txState == VWTP_IDLE)
    {
        tpParams[0u] = 0xA1u; // params resp
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
            msg[0] = 0x10u | chPtr->seqCntTx; // single frame or last frame
            for (tmp=0;tmp<(dlc-1);tmp++)
            {
                msg[tmp+1] = chPtr->txBuffer[tmp+(chPtr->txOffset)];
            }
            VWTP_SENDMESSAGE(chPtr->cfg.txId,dlc,msg);
            chPtr->txState = VWTP_FINISHED;
        }
        else
        {
            dlc = 8;
            msg[0] = 0x20u | chPtr->seqCntTx; // consecutive frame
            for (tmp=0;tmp<7u;tmp++)
            {
                msg[tmp+1u] = chPtr->txBuffer[tmp+(chPtr->txOffset)];
            }
            VWTP_SENDMESSAGE(chPtr->cfg.txId,dlc,msg);
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

