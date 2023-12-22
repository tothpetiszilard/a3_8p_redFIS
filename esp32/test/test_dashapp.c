#include "unity.h"
#include <stdlib.h>
#include <stdbool.h>
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

#include "dashapp.c"

#include "mock_sysStates.h"
#include "mock_vwtp.h"

void setup(void)
{

}

void tearDown(void)
{

}

void test_DashApp_Init(void)
{
    appState = DASHAPP_SHUTDOWN;
    waitForAck = 1;
    retryCnt = 3;
    DashApp_Init();
    TEST_ASSERT_EQUAL(DASHAPP_INIT, appState);
    TEST_ASSERT_EQUAL(0, waitForAck);
    TEST_ASSERT_EQUAL(0, retryCnt);
}


void test_DashApp_GetStatus(void)
{
    DashApp_ReturnType expectedRetVals[DASHAPP_SHUTDOWN+1] = 
    {
        DASHAPP_ERR,//DASHAPP_INIT=0,
        DASHAPP_ERR,//DASHAPP_PWRSTATE=1,
        DASHAPP_ERR,//DASHAPP_IDREQ=2,
        DASHAPP_ERR,//DASHAPP_PAGEREQ=3,
        DASHAPP_ERR,//DASHAPP_GETSTATUS=4,
        DASHAPP_ERR,//DASHAPP_SEND2F=5,
        DASHAPP_ERR,//DASHAPP_PREWRITE=6,
        DASHAPP_ERR,//DASHAPP_SHOW=7,
        DASHAPP_BUSY,//DASHAPP_WRITE=8,
        DASHAPP_OK,//DASHAPP_READY=9,
        DASHAPP_ERR,//DASHAPP_CLEAR = 10,
        DASHAPP_ERR,//DASHAPP_WAIT,
        DASHAPP_PAUSE,//DASHAPP_SUSPEND,
        DASHAPP_ERR //DASHAPP_SHUTDOWN
    };
    DashApp_ReturnType retVal;
    for (uint8_t i=0; i < sizeof(expectedRetVals); i++)
    {
        retVal = DASHAPP_ERR;
        appState = (DashApp_StateType)i;
        retVal = DashApp_GetStatus();
        TEST_ASSERT_EQUAL(expectedRetVals[i], retVal);
    }
}

