#include "unity.h"
#include <stdlib.h>
#define INC_FREERTOS_H
#define INC_TASK_H
#define portTICK_PERIOD_MS (1000u / 200u)

typedef uint32_t TaskHandle_t;
typedef void (* TaskFunction_t)( void * );

uint32_t xTaskCreatePinnedToCore( TaskFunction_t pvTaskCode,
                                        const char * const pcName,
                                        const uint32_t usStackDepth,
                                        void * const pvParameters,
                                        uint32_t uxPriority,
                                        TaskHandle_t * const pvCreatedTask,
                                        const uint32_t xCoreID)
                                        {
                                            *pvCreatedTask = 0x55AA;
                                            return 0;
                                        }
void vTaskSuspendAll(void)
{

}
void xTaskResumeAll(void)
{

}
void vTaskDelay(uint32_t delay)
{

}

#include "vwtp.c"
#include "vwtp_cfg.c"

#include "mock_canWrapper.h"
#include "mock_dashapp.h"
#include "mock_navapp.h"
#include "mock_kwp.h"
#include <stdbool.h>

void setup(void)
{

}

void tearDown(void)
{

}

void test_VwTp_Init(void)
{
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        vwtp_channels[i].txState = VWTP_FINISHED; // Set it to wrong
        vwtp_channels[i].rxState = VWTP_FINISHED; // Set it to wrong
        vwtp_channels[i].seqCntRx = 0; // Init should set if to 0xF
    }
    VwTp_Init(); // Call SUT

    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        TEST_ASSERT_EQUAL(VWTP_CONNECT, vwtp_channels[i].txState);
        TEST_ASSERT_EQUAL(VWTP_CONNECT, vwtp_channels[i].rxState);
        TEST_ASSERT_EQUAL(0xF, vwtp_channels[i].seqCntRx);
        TEST_ASSERT_EQUAL(0, vwtp_channels[i].seqCntTx); // Shall remain 0
        TEST_ASSERT_EQUAL(0, vwtp_channels[i].ackSeqCntTx); // Shall remain 0
        TEST_ASSERT_EQUAL(0, vwtp_channels[i].ackSeqCntRx); // Shall remain 0
    }
}

void test_VwTp_Connect(void)
{
    VwTp_ReturnType result = VWTP_ERR;
    bool diag_configured = false;
    VwTp_ChannelType * chPtr= NULL;
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        if (VWTP_DIAG == vwtp_channels[i].cfg.mode)
        {
            diag_configured = true;
            vwtp_channels[i].cfg.txId = 0x200u;
            vwtp_channels[i].txState = VWTP_CONNECT;
            chPtr = &vwtp_channels[i];
            break;
        }
    }
    if (true == diag_configured)
    {

        Can_Write_ExpectAndReturn((uint16_t)0x200u,(uint8_t)7u,(uint8_t *)NULL,(uint8_t)CAN_OK);
        Can_Write_IgnoreArg_dataPtr();
        result = VwTp_Connect(0x01u);
        TEST_ASSERT_EQUAL(0x201u, chPtr->cfg.rxId);
        TEST_ASSERT_EQUAL(VWTP_OK, result);
    }
    else
    {
        result = VwTp_Connect(0x01u);
        TEST_ASSERT_EQUAL(VWTP_ERR, result);
    }
}

void test_VwTp_HandleTxTimeout_Normal(void)
{
    VwTp_ChannelType * chPtr= NULL;
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->txState = VWTP_IDLE;
        chPtr->txTimeout = 1;
        VwTp_HandleTxTimeout(chPtr);
        TEST_ASSERT_EQUAL(0, chPtr->txTimeout);
    }
}

void test_VwTp_HandleTxTimeout_Increment(void)
{
    VwTp_ChannelType * chPtr= NULL;
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->txState = VWTP_ACK;
        chPtr->txTimeout = 0;
        VwTp_HandleTxTimeout(chPtr);
        TEST_ASSERT_EQUAL(1u , chPtr->txTimeout);
    }
}

void test_VwTp_HandleTxTimeout_ResendAck(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t maxTime = 0;
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        if (0x80 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 10 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu); // we are already in 10ms scale
        }
        else if (0x40 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 1 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu)/10u; // we are in 10ms scale
        }
        else if (0xC0 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 100 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu) * 10u; // we are in 10ms scale
        }
        chPtr->txState = VWTP_ACK;
        chPtr->txTimeout = maxTime;
        //chPtr->ackSeqCntTx = 0x9;
        //chPtr->seqCntTx = 0xA;
        chPtr->txSize = 0;
        VwTp_HandleTxTimeout(chPtr);
        TEST_ASSERT_EQUAL(0 , chPtr->txOffset);
        TEST_ASSERT_EQUAL(0, chPtr->txTimeout);
        //TEST_ASSERT_EQUAL(chPtr->ackSeqCntTx, chPtr->seqCntTx);
        TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->txState);
    }
}

void test_VwTp_HandleTxTimeout_ResendData1(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t maxTime = 0;
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        if (0x80 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 10 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu); // we are already in 10ms scale
        }
        else if (0x40 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 1 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu)/10u; // we are in 10ms scale
        }
        else if (0xC0 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 100 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu) * 10u; // we are in 10ms scale
        }
        chPtr->txState = VWTP_ACK;
        chPtr->txTimeout = maxTime;
        chPtr->ackSeqCntTx = 0x9;
        chPtr->seqCntTx = 0xA;
        chPtr->txSize = 10;
        VwTp_HandleTxTimeout(chPtr);
        TEST_ASSERT_EQUAL(0 , chPtr->txOffset);
        TEST_ASSERT_EQUAL(0, chPtr->txTimeout);
        TEST_ASSERT_EQUAL(chPtr->ackSeqCntTx, chPtr->seqCntTx);
        TEST_ASSERT_EQUAL(VWTP_WAIT, chPtr->txState);
    }
}

void test_VwTp_HandleTxTimeout_ResendData2(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t maxTime = 0;
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        if (0x80 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 10 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu); // we are already in 10ms scale
        }
        else if (0x40 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 1 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu)/10u; // we are in 10ms scale
        }
        else if (0xC0 == (chPtr->cfg.ackTimeout & 0xC0u))
        {
            // multiplier: 100 ms
            maxTime = (chPtr->cfg.ackTimeout & 0x3Fu) * 10u; // we are in 10ms scale
        }
        chPtr->txState = VWTP_ACK;
        chPtr->txTimeout = maxTime + 10;
        chPtr->ackSeqCntTx = 0x9;
        chPtr->seqCntTx = 0xA;
        chPtr->txSize = 10;
        VwTp_HandleTxTimeout(chPtr);
        TEST_ASSERT_EQUAL(0 , chPtr->txOffset);
        TEST_ASSERT_EQUAL(0, chPtr->txTimeout);
        TEST_ASSERT_EQUAL(chPtr->ackSeqCntTx, chPtr->seqCntTx);
        TEST_ASSERT_EQUAL(VWTP_WAIT, chPtr->txState);
    }
}

void test_VwTp_Send_OK(void)
{
    VwTp_ChannelType * chPtr= NULL;
    VwTp_ReturnType retVal = VWTP_ERR;
    uint8_t msgToSend[VWTP_TXBUFFERSIZE];
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        for(uint32_t cnt=0;cnt < sizeof(chPtr->txBuffer);cnt++)
        {
            chPtr = &vwtp_channels[i];
            for(uint32_t s=0;s < cnt;s++)
            {
                msgToSend[s] = (uint8_t)rand();
            }
            chPtr->txSize = 0;
            chPtr->txOffset = 255;
            chPtr->txState = VWTP_IDLE;
            retVal = VwTp_Send(i,msgToSend,cnt);
            TEST_ASSERT_EQUAL(VWTP_OK, retVal);
            TEST_ASSERT_EQUAL(cnt, chPtr->txSize);
            TEST_ASSERT_EQUAL(0, chPtr->txOffset);
            TEST_ASSERT_EQUAL(VWTP_WAIT, chPtr->txState);
            for(uint32_t v=0;v < cnt;v++)
            {
                TEST_ASSERT_EQUAL(msgToSend[v], chPtr->txBuffer[v]);
            }
        }
    }
}

void test_VwTp_Send_StateConnect(void)
{
    VwTp_ChannelType * chPtr = NULL;
    VwTp_ReturnType retVal = VWTP_ERR;
    uint8_t msgToSend[VWTP_TXBUFFERSIZE];
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->txSize = 0;
        chPtr->txOffset = 0;
        chPtr->txFlags.params = 0;
        chPtr->txState = VWTP_CONNECT;
        chPtr->rxState = VWTP_CONNECT;
        retVal = VwTp_Send(i,msgToSend,0xAA);
        if (VWTP_DIAG != chPtr->cfg.mode)
        {
            TEST_ASSERT_EQUAL(VWTP_PENDING, retVal);
            TEST_ASSERT_EQUAL(VWTP_TPPARAMS_REQUEST, chPtr->txFlags.params);
            TEST_ASSERT_EQUAL(0, chPtr->txSize);
            TEST_ASSERT_EQUAL(0, chPtr->txOffset);
            TEST_ASSERT_EQUAL(VWTP_CONNECT, chPtr->txState);
            TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->rxState);
        }
        else
        {
            TEST_ASSERT_EQUAL(0, chPtr->txFlags.params);
            TEST_ASSERT_EQUAL(0, chPtr->txSize);
            TEST_ASSERT_EQUAL(0, chPtr->txOffset);
            TEST_ASSERT_EQUAL(VWTP_CONNECT, chPtr->txState);
            TEST_ASSERT_EQUAL(VWTP_CONNECT, chPtr->rxState);
        }
    }
}

void test_VwTp_Send_SizeError(void)
{
    VwTp_ChannelType * chPtr= NULL;
    VwTp_ReturnType retVal = VWTP_ERR;
    uint8_t msgToSend[VWTP_TXBUFFERSIZE];
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        for(uint32_t cnt = (1 + sizeof(chPtr->txBuffer));cnt < UINT16_MAX;cnt++)
        {
            chPtr = &vwtp_channels[i];
            chPtr->txSize = 0;
            chPtr->txOffset = 0x55;
            chPtr->txState = VWTP_IDLE;
            retVal = VwTp_Send(i,msgToSend,cnt);
            TEST_ASSERT_EQUAL(VWTP_ERR, retVal);
            TEST_ASSERT_EQUAL(0, chPtr->txSize);
            TEST_ASSERT_EQUAL(0x55, chPtr->txOffset);
            TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->txState);
        }
    }
}

void test_VwTp_Send_StateError(void)
{
    VwTp_ChannelType * chPtr= NULL;
    VwTp_ReturnType retVal = VWTP_ERR;
    uint8_t msgToSend[VWTP_TXBUFFERSIZE];
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        for(uint32_t cnt = (1 + sizeof(chPtr->txBuffer));cnt < UINT16_MAX;cnt++)
        {
            chPtr = &vwtp_channels[i];
            chPtr->txSize = 25;
            chPtr->txOffset = 0x55;
            chPtr->txState = VWTP_WAIT;
            retVal = VwTp_Send(i,msgToSend,cnt);
            TEST_ASSERT_EQUAL(VWTP_ERR, retVal);
            TEST_ASSERT_EQUAL(25, chPtr->txSize);
            TEST_ASSERT_EQUAL(0x55, chPtr->txOffset);
            TEST_ASSERT_EQUAL(VWTP_WAIT, chPtr->txState);
        }
    }
}

void test_VwTp_Receive_DiagConnect(void)
{
    bool diag_configured = false;
    uint8_t msgRx[7] = 
    {
        0x00u,
        0xD0u,
        0x00u,
        0x03u,
        0x40u,
        0x07u,
        0x01u
    };
    VwTp_ChannelType * chPtr= NULL;
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        if (VWTP_DIAG == vwtp_channels[i].cfg.mode)
        {
            diag_configured = true;
            vwtp_channels[i].cfg.txId = 0x200u;
            vwtp_channels[i].txState = VWTP_CONNECT;
            chPtr = &vwtp_channels[i];
            break;
        }
    }
    if(true == diag_configured)
    {
        chPtr->rxState = VWTP_CONNECT;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(0x300u, chPtr->cfg.rxId);
        TEST_ASSERT_EQUAL(0x740u, chPtr->cfg.txId);
        TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->rxState);
        TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->txState);
        TEST_ASSERT_EQUAL(VWTP_TPPARAMS_REQUEST, chPtr->txFlags.params);
    }
}

void test_VwTp_Receive_A1(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[6] = 
    {
        0xA1, 0x0F, 0x8A, 0xFF, 0x4A, 0xFF
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(VWTP_FINISHED, chPtr->txState);
    }
}

void test_VwTp_Receive_A3(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[1] = 
    {
        0xA3
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->txState = VWTP_IDLE;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(VWTP_TPPARAMS_RESPONSE, chPtr->txFlags.params);
        TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->txState);
    }
}

void test_VwTp_Receive_A0(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[6] = 
    {
        0xA0, 0x0F, 0x8A, 0xFF, 0x32, 0xFF
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->txState = VWTP_CONNECT;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(VWTP_TPPARAMS_RESPONSE, chPtr->txFlags.params);
        TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->txState);
    }
}

void test_VwTp_Receive_SF(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[5] = 
    {
        0x10, 0x00, 0x02, 0x10, 0x89
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->rxState = VWTP_IDLE;
        chPtr->seqCntRx = 0xFu;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(0, chPtr->seqCntRx);
        TEST_ASSERT_EQUAL(sizeof(msgRx)-1u, chPtr->rxSize);
        TEST_ASSERT_EQUAL(1u, chPtr->txFlags.ack);
        TEST_ASSERT_EQUAL(VWTP_ACK, chPtr->rxState);
        for (uint8_t dlc=0; dlc < chPtr->rxSize; dlc++)
        {
            TEST_ASSERT_EQUAL(msgRx[dlc+1], chPtr->rxBuffer[dlc]);
        }
    }
}

void test_VwTp_Receive_FF(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[8] = 
    {
        0x22, 0x00, 0x30, 0x5A, 0x9B, 0x30, 0x33, 0x47
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->rxState = VWTP_IDLE;
        chPtr->seqCntRx = 0x1u;
        chPtr->rxSize = 0;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(2u, chPtr->seqCntRx);
        TEST_ASSERT_EQUAL(sizeof(msgRx)-1u, chPtr->rxSize);
        TEST_ASSERT_EQUAL(VWTP_WAIT, chPtr->rxState);
        for (uint8_t dlc=0; dlc < chPtr->rxSize; dlc++)
        {
            TEST_ASSERT_EQUAL(msgRx[dlc+1], chPtr->rxBuffer[dlc]);
        }
    }
}

void test_VwTp_Receive_CF(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[8] = 
    {
        0x23, 0x39, 0x30, 0x36, 0x30, 0x32, 0x31, 0x4A
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->rxState = VWTP_WAIT;
        chPtr->seqCntRx = 0x2u;
        chPtr->rxSize = 7u;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(3u, chPtr->seqCntRx);
        TEST_ASSERT_EQUAL(7u + (sizeof(msgRx)-1u), chPtr->rxSize);
        TEST_ASSERT_EQUAL(VWTP_WAIT, chPtr->rxState);
        for (uint8_t dlc=0; dlc < (sizeof(msgRx)-1); dlc++)
        {
            TEST_ASSERT_EQUAL(msgRx[dlc+1], chPtr->rxBuffer[7u + dlc]);
        }
    }
}

void test_VwTp_Receive_ACK(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[1] = 
    {
        0xB4u
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->txState = VWTP_ACK;
        chPtr->seqCntRx = 0x3u;
        chPtr->txSize = 7;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(4u, chPtr->seqCntTx);
        TEST_ASSERT_EQUAL(0u, chPtr->txSize);
        TEST_ASSERT_EQUAL(0u, chPtr->txOffset);
        TEST_ASSERT_EQUAL(VWTP_FINISHED, chPtr->txState);
    }
}

void test_VwTp_Receive_Close(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[1] = 
    {
        0xA8u
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        Can_Write_ExpectAndReturn(chPtr->cfg.txId,(uint8_t)1u,(uint8_t *)NULL,(uint8_t)CAN_OK);
        Can_Write_IgnoreArg_dataPtr();
        if (NULL != chPtr->cfg.txConfirmation)
        {
            if (DashApp_TxConfirmation == chPtr->cfg.txConfirmation)
            {
                DashApp_TxConfirmation_Expect(VWTP_ERR);
            }
            else if (Kwp_TxConfirmation == chPtr->cfg.txConfirmation)
            {
                Kwp_TxConfirmation_Expect(VWTP_ERR);
            }
            else if (NavApp_TxConfirmation == chPtr->cfg.txConfirmation)
            {
                NavApp_TxConfirmation_Expect(VWTP_ERR);
            }
            else 
            {
                /* Unknown */
            }
            
        }
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(0u, chPtr->seqCntTx);
        TEST_ASSERT_EQUAL(0xFu, chPtr->seqCntRx);
        TEST_ASSERT_EQUAL(0u, chPtr->txSize);
        TEST_ASSERT_EQUAL(0u, chPtr->rxSize);
        TEST_ASSERT_EQUAL(VWTP_CONNECT, chPtr->txState);
        if (VWTP_DIAG == chPtr->cfg.mode)
        {
            TEST_ASSERT_EQUAL(VWTP_CONNECT, chPtr->rxState);
            TEST_ASSERT_EQUAL(0x200, chPtr->cfg.txId);
            TEST_ASSERT_EQUAL(0, chPtr->cfg.rxId);
        }
        else
        {
            TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->rxState);
        }
    }
}

void test_VwTp_Receive_Break_NotZero(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[1] = 
    {
        0xA4u
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->rxState = VWTP_IDLE;
        chPtr->txState = VWTP_ACK;
        chPtr->txOffset = 0x3u;
        chPtr->ackSeqCntTx = 3;
        chPtr->seqCntTx = 4;
        chPtr->txSize = 7;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(0u, chPtr->txOffset);
        TEST_ASSERT_EQUAL(chPtr->ackSeqCntTx, chPtr->seqCntTx);
        TEST_ASSERT_EQUAL(VWTP_WAIT, chPtr->txState);
    }
}

void test_VwTp_Receive_Break_Zero(void)
{
    VwTp_ChannelType * chPtr= NULL;
    uint8_t msgRx[1] = 
    {
        0xA4u
    };
    for (uint32_t i = 0;i < (sizeof(vwtp_channels)/sizeof(vwtp_channels[0]));i++ )
    {
        chPtr = &vwtp_channels[i];
        chPtr->rxState = VWTP_IDLE;
        chPtr->txState = VWTP_WAIT;
        chPtr->txOffset = 0x3u;
        chPtr->txSize = 0;
        VwTp_Receive(chPtr->cfg.rxId,sizeof(msgRx),msgRx);
        TEST_ASSERT_EQUAL(0u, chPtr->txOffset);
        TEST_ASSERT_EQUAL(VWTP_IDLE, chPtr->txState);
    }
}

