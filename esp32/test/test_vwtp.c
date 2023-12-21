#include "unity.h"
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


