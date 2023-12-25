#include "vwtp.h"
#include "vwtp_cfg.h"
#include "stddef.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef VWTP_DET
#include "esp_log.h"
#endif

static TaskHandle_t VwTpTaskHdl = NULL;

static void VwTp_HandleTx(VwTp_ChannelType * const chPtr);
static void VwTp_HandleTxTimeout(VwTp_ChannelType * const chPtr);
static void VwTp_HandleRx(VwTp_ChannelType * chPtr,uint8_t dlc,uint8_t * dataPtr);
static void VwTp_HandleRxTimeout(VwTp_ChannelType * const chPtr);
static void VwTp_HandleConnect(VwTp_ChannelType * chPtr,uint8_t * dataPtr);
static void VwTp_HandleCallbacks(VwTp_ChannelType * const chPtr);
static void VwTp_sendTpParams(VwTp_ChannelType * const chPtr, uint8_t response);
static void VwTp_sendClose(VwTp_ChannelType * const chPtr);
static void VwTp_sendAck(VwTp_ChannelType * const chPtr);
static void VwTp_sendBreak(VwTp_ChannelType * const chPtr);


void VwTp_Init(void)
{
    uint8_t i = 0;
    for (i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        vwtp_channels[i].txState = VWTP_CONNECT;
        vwtp_channels[i].rxState = VWTP_CONNECT;
        vwtp_channels[i].seqCntRx = 0xFu; // 0 is expected in the first frame
    }
    #ifndef REDFIS_SINGLE_THREAD
    xTaskCreatePinnedToCore(VwTp_Cyclic, "VwTp", 2048u, NULL, 5, &VwTpTaskHdl,1);
    #endif
    //TODO: 0x9, ACK, not ready for next packet: should be handled better
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

static void VwTp_HandleTxTimeout(VwTp_ChannelType * const chPtr)
{
    uint8_t ackCfg = 0;
    if (VWTP_ACK == chPtr->txState)
    {
        if (0x80 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 10 ms
            ackCfg = (chPtr->cfg.ackTimeout & 0x3Fu); // we are already in 10ms scale
        }
        else if (0x40 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 1 ms
            ackCfg = (chPtr->cfg.ackTimeout & 0x3Fu)/10u; // we are in 10ms scale
        }
        else if (0xC0 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 100 ms
            ackCfg = (chPtr->cfg.ackTimeout & 0x3Fu) * 10u; // we are in 10ms scale
        }
        if (chPtr->txTimeout >= ackCfg)
        {
            // timeout, resend
            #ifdef VWTP_DET
            ESP_LOGI("VWTP","Timeout, resend: id: %x , seq: %x",chPtr->cfg.txId,chPtr->seqCntTx);
            #endif
            vTaskSuspendAll(); // Critical section, interrupts enabled
            chPtr->txOffset = 0;
            if (chPtr->txSize > 0)
            {
                chPtr->seqCntTx = chPtr->ackSeqCntTx; // set back the seq cntr
                chPtr->txState = VWTP_WAIT;
            }
            else
            {
                chPtr->txState = VWTP_IDLE;
            }
            chPtr->txTimeout = 0;
            xTaskResumeAll(); // End of critical section, interrupts enabled
            
        }
        else
        {
            // waiting for ACK
            chPtr->txTimeout++;
        }
    }
    else
    {
        // Normal communication
        chPtr->txTimeout = 0;
    }
}

void VwTp_Cyclic(void *pvParameters)
{
    uint8_t chId = 0;
    VwTp_ChannelType * chPtr = NULL;
    #if (1 == CONFIG_BENCH_TEST_MODE)
    const uint8_t ignition [4] = {7,20,0,0};
    uint8_t callCounter = 0;
    #endif
    #ifndef REDFIS_SINGLE_THREAD
    while(1)
    #endif
    {
        for (chId = 0;chId < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));chId++ )
        {
            chPtr = &vwtp_channels[chId];
            VwTp_HandleTxTimeout(chPtr);
            VwTp_HandleTx(chPtr); // TODO: Requesting ACK after x transmitted frames 
            VwTp_HandleCallbacks(chPtr);
            VwTp_HandleRxTimeout(chPtr);
            #ifndef REDFIS_SINGLE_THREAD
            //vTaskDelay((10u/(sizeof(vwtp_channels)/sizeof(vwtp_channels[0]))) / portTICK_PERIOD_MS);
            vTaskDelay( 5u / portTICK_PERIOD_MS);
            #endif
        }
        #if (1 == CONFIG_BENCH_TEST_MODE)
        if (callCounter >= 20)
        {
            VWTP_SENDMESSAGE(0x575, 4, (uint8_t *)ignition);
            callCounter = 0;
        }
        else 
        {
            callCounter++;
        }
        #endif
    }
}

VwTp_ReturnType VwTp_Send(uint8_t chId, uint8_t * buffer, uint16_t len)
{
    VwTp_ChannelType * chPtr = NULL;
    VwTp_ReturnType retVal = VWTP_ERR;
    uint16_t i = 0;
    if (chId < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0])))
    {
        chPtr = &vwtp_channels[chId];
        vTaskSuspendAll(); // Critical section, interrupts enabled
        if ((chPtr->txState == VWTP_IDLE) && (sizeof(chPtr->txBuffer) >= len))
        {
            chPtr->txSize = len;
            chPtr->txOffset = 0u;
            for (i=0; i<len; i++)
            {
                chPtr->txBuffer[i] = buffer[i];
            }
            chPtr->txState = VWTP_WAIT;
            retVal = VWTP_OK;
        }
        else if ((chPtr->txState == VWTP_CONNECT) && (chPtr->cfg.mode != VWTP_DIAG ))
        {
            // Try to open channel
            chPtr->txFlags.params = VWTP_TPPARAMS_REQUEST;
            chPtr->rxState = VWTP_IDLE;
            retVal = VWTP_PENDING;
        }
        else 
        {
            // Do nothing, return error
        }
        xTaskResumeAll(); // End of critical section, interrupts enabled
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
        if ((chPtr->rxState == VWTP_CONNECT) && (chPtr->cfg.mode == VWTP_DIAG ))
        {
            VwTp_HandleConnect(chPtr,dataPtr);
        }
        else 
        {
            VwTp_HandleRx(chPtr,dlc,dataPtr);
        }
    }
}

static void VwTp_HandleRxTimeout(VwTp_ChannelType * const chPtr)
{
    if (VWTP_WAIT == chPtr->rxState)
    {
        if(20u <= chPtr->rxTimeout)
        {
            // Send break?
            chPtr->txFlags.brk = 1u;
            chPtr->rxState = VWTP_IDLE;
            chPtr->seqCntRx = chPtr->ackSeqCntRx;
            chPtr->rxTimeout = 0u;
        }
        else 
        {
            chPtr->rxTimeout++;
        }
    }
    else 
    {
        chPtr->rxTimeout = 0u;
    }
}

void VwTp_Disconnect(uint8_t chId)
{
    VwTp_ChannelType * chPtr = NULL;
    if (chId < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0])))
    {
        chPtr = &vwtp_channels[chId];
        VwTp_sendClose(chPtr);
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
        chPtr->txFlags.params = VWTP_TPPARAMS_REQUEST;
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
}

static void VwTp_HandleRx(VwTp_ChannelType * chPtr,uint8_t dlc,uint8_t * dataPtr)
{
    uint8_t i = 0;
    if ((0xA3u == dataPtr[0]) || (0xA0u == dataPtr[0]))
    {
        // Connection init or alive check
        if (chPtr->txState == VWTP_CONNECT)
        {
            chPtr->txState = VWTP_IDLE;
        }
        chPtr->txFlags.params = VWTP_TPPARAMS_RESPONSE;
    }
    else if (0xA1u == dataPtr[0])
    {
        // Connection initialized or response to alive check
        chPtr->txState = VWTP_FINISHED; // notify app
    }
    else if (0x10u == (dataPtr[0] & 0xF0u))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        if ((dataPtr[0] & 0x0Fu) == ((chPtr->seqCntRx + 1u) & 0x0Fu))
        {
            // seq cnt OK
            chPtr->seqCntRx = (dataPtr[0] & 0xF);
            //last frame or single frame
            if (VWTP_IDLE == chPtr->rxState)
            {
                //single frame received
                chPtr->rxSize = (dlc-1);
                for(i=0;i<chPtr->rxSize;i++)
                {
                    chPtr->rxBuffer[i] = dataPtr[i+1];
                }
                chPtr->txFlags.ack = 1u; // Ack + rxIndication
                chPtr->rxState = VWTP_ACK;
            }
            else if (VWTP_WAIT == chPtr->rxState )
            {
                // last frame received
                for(i=0;i<(dlc-1);i++)
                {
                    chPtr->rxBuffer[chPtr->rxSize + i] = dataPtr[i+1];
                }
                chPtr->rxSize += (dlc-1);
                chPtr->txFlags.ack = 1u; // Ack + rxIndication
                chPtr->rxState = VWTP_ACK;
            }
            else 
            {
                // received single/last data frame while not expected
                #ifdef VWTP_DET
                ESP_LOGE("VWTP","Not expected s/l data frame, id %x, seq %x", chPtr->cfg.rxId, dataPtr[0]);
                #endif
                VwTp_sendClose(chPtr);
            }
        }
        else 
        {
            // Wrong sequence number received
            // ESP_LOGE("VWTP","sl wrong seq num");
            if (VWTP_IDLE == chPtr->rxState)
            {
                chPtr->seqCntRx = ((dataPtr[0]-1u) & 0x0Fu); // resync
            }
        }
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    else if ((0x20u == (dataPtr[0] & 0xF0u)) || (0u == (dataPtr[0] & 0xF0u)))
    {
        if ((VWTP_IDLE == chPtr->rxState) || (VWTP_WAIT == chPtr->rxState ))
        {
            vTaskSuspendAll(); // Critical section, interrupts enabled
            if ((dataPtr[0] & 0x0Fu) == ((chPtr->seqCntRx + 1u) & 0x0Fu))
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
                    chPtr->txFlags.ack = 1u; // Ack + rxIndication
                }
            }
            else 
            {
                // Wrong sequence number received
                #ifdef VWTP_DET
                ESP_LOGE("VWTP","fc wrong seq num");
                #endif
            }
            xTaskResumeAll(); // End of critical section, interrupts enabled
        }
        else 
        {
            //Not expected first or consecutive data frame received
            #ifdef VWTP_DET
            ESP_LOGE("VWTP","Not expected f/c data frame, id %x, seq %x", chPtr->cfg.rxId, dataPtr[0]);
            #endif
            VwTp_sendClose(chPtr);
        }
    }
    else if ((0xB0u == (dataPtr[0] & 0xF0u)) || (0x90u == (dataPtr[0] & 0xF0u)))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        if (VWTP_ACK == chPtr->txState)
        {
            chPtr->seqCntTx = (dataPtr[0] & 0x0Fu);
            chPtr->ackSeqCntTx = (dataPtr[0] & 0x0Fu);
            chPtr->txSize = 0;
            chPtr->txOffset = 0;
            chPtr->txState = VWTP_FINISHED; // txConfirmation
        }
        else 
        {
            //received ack when not expected
            #ifdef VWTP_DET
            ESP_LOGE("VWTP","Not expected ACK");
            #endif
        }
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    else if (0xA8u == dataPtr[0])
    {
        // Connection was terminated
        if (VWTP_CONNECT != chPtr->txState)
        {
            VwTp_sendClose(chPtr);
        }
    }
    else if (0xA4u == dataPtr[0])
    {
        // Break
        #ifdef VWTP_DET
        ESP_LOGI("VWTP","Break");
        #endif
        if ((VWTP_WAIT == chPtr->txState) || (VWTP_ACK == chPtr->txState))
        {
            // Resend
            #ifdef VWTP_DET
            ESP_LOGI("VWTP","Resend");
            #endif
            vTaskSuspendAll(); // Critical section, interrupts enabled
            chPtr->txOffset = 0;
            if (chPtr->txSize > 0)
            {
                chPtr->seqCntTx = chPtr->ackSeqCntTx;
                chPtr->txState = VWTP_WAIT;
            }
            else
            {
                chPtr->txState = VWTP_IDLE;
            }
            xTaskResumeAll(); // End of critical section, interrupts enabled
        }
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
    ackSeq = (chPtr->seqCntRx+1) & 0x0Fu;
    msg[0] = 0xB0u | ackSeq; // ack
    if (CAN_OK == VWTP_SENDMESSAGE(chPtr->cfg.txId,sizeof(msg),msg))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        chPtr->ackSeqCntRx = chPtr->seqCntRx;
        chPtr->txFlags.ack = 0u;
        // reset states to receive the next msg
        chPtr->rxState = VWTP_FINISHED;
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
}

static void VwTp_sendClose(VwTp_ChannelType * const chPtr)
{
    uint8_t tpClose[1] = {0xA8u};
    
    vTaskSuspendAll(); // Critical section, interrupts enabled
    chPtr->seqCntTx = 0;
    chPtr->seqCntRx = 0xFu;
    chPtr->rxSize = 0;
    chPtr->txSize = 0;
    chPtr->rxState = VWTP_IDLE;
    chPtr->txState = VWTP_CONNECT;
    xTaskResumeAll(); // End of critical section, interrupts enabled
    VWTP_SENDMESSAGE(chPtr->cfg.txId,sizeof(tpClose),tpClose);
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
    if (NULL != chPtr->cfg.txConfirmation)
    {
        chPtr->cfg.txConfirmation(VWTP_ERR);
    }
}

static void VwTp_sendTpParams(VwTp_ChannelType * const chPtr, uint8_t response)
{
    uint8_t tpParams[6];
    
    if (response < 2)
    {
        tpParams[0u] = 0xA0u | response; // params resp
        tpParams[1u] = chPtr->cfg.blockSize; // msg num before ack
        tpParams[2u] = chPtr->cfg.ackTimeout; // timing
        tpParams[3u] = 0xFFu; // dummy
        tpParams[4u] = chPtr->cfg.ips; // timing2
        tpParams[5u] = 0xFFu; // dummy
        if (CAN_OK == VWTP_SENDMESSAGE(chPtr->cfg.txId,sizeof(tpParams),tpParams))
        {
            chPtr->txFlags.params = 0;
        }
    }
}

static void VwTp_sendBreak(VwTp_ChannelType * const chPtr)
{
    uint8_t msg[1];
    
    msg[0] = 0xA4u; // Break
    if (CAN_OK == VWTP_SENDMESSAGE(chPtr->cfg.txId,sizeof(msg),msg))
    {
        chPtr->txFlags.brk = 0;
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
        vTaskSuspendAll(); // Critical section, interrupts enabled
        chPtr->rxSize = 0u;
        chPtr->rxState = VWTP_IDLE;
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    if (chPtr->txState == VWTP_FINISHED)
    {
        if (NULL != chPtr->cfg.txConfirmation)
        {
            chPtr->cfg.txConfirmation(VWTP_OK);
        }
        chPtr->txState = VWTP_IDLE;
    }
}

static void VwTp_HandleTx(VwTp_ChannelType * const chPtr)
{
    uint16_t tmp;
    uint8_t dlc;
    uint8_t msg[8];
    if ( VWTP_WAIT == chPtr->txState )
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
                    chPtr->txState = VWTP_FINISHED;
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
    else if ((VWTP_IDLE == chPtr->txState) && (0 != chPtr->txFlags.ack))
    {
        VwTp_sendAck(chPtr);
    }
    else if ((VWTP_IDLE == chPtr->txState) && (0 != chPtr->txFlags.brk))
    {
        VwTp_sendBreak(chPtr);
    }
    else if (((VWTP_IDLE == chPtr->txState) || (VWTP_CONNECT == chPtr->txState)) && (0 != chPtr->txFlags.params))
    {
        VwTp_sendTpParams(chPtr, (chPtr->txFlags.params & VWTP_TPPARAMS_MASK));
    }
    else
    {
        // No task to do
    }
}

