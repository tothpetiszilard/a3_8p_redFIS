#include "diagcore.h"
#include "kwp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t taskHandle;

typedef enum
{
    DIAG_IDLE,
    DIAG_REQ,
    DIAG_READ,
    DIAG_READY
}Diag_State;

static Diag_State state = DIAG_IDLE;
static uint8_t actualDid = 0;
static Diag_CallbackType *callback;

Diag_ReturnType Diag_ReqDid(uint8_t did)
{
    Diag_ReturnType retVal = DIAG_ERR;
    if (DIAG_IDLE == state)
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        actualDid = did;
        state = DIAG_REQ;
        retVal = DIAG_OK;
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    return retVal;
}


void Diag_Cyclic(void *pvParameters)
{
    static uint8_t errCnt = 0;
    uint8_t didBuffer[4u*3u];
    #ifndef REDFIS_SINGLE_THREAD
    while(1)
    #endif
    {
        #ifndef REDFIS_SINGLE_THREAD
        vTaskDelay(50u / portTICK_PERIOD_MS);
        #endif
        switch(state)
        {
            case DIAG_IDLE:
            break;
            case DIAG_REQ:
            if (KWP_OK == Kwp_RequestData(actualDid))
            {
                state = DIAG_READ;
            }
            break;
            case DIAG_READ:
            if (KWP_OK == Kwp_GetDataFromECU(didBuffer))
            {
                if (NULL != callback)
                {
                    callback(didBuffer, actualDid);
                }
                state = DIAG_IDLE;
            }
            else 
            {
                if (errCnt > 10)
                {
                    errCnt = 0;
                    state = DIAG_REQ; // re-request
                }
                else 
                {
                    errCnt++;
                }
            }
            break;
            default:
            break;
        }
        #ifndef REDFIS_SINGLE_THREAD
        vTaskDelay(50u / portTICK_PERIOD_MS);
        #endif
    }
}


Diag_ReturnType Diag_Init(uint8_t ecuId, Diag_CallbackType *cb)
{
    Diag_ReturnType retVal = DIAG_ERR;
    #ifndef REDFIS_SINGLE_THREAD
    vTaskDelay(210u / portTICK_PERIOD_MS);
    #endif
    if (KWP_OK == Kwp_Init(ecuId))
    {
        state = DIAG_IDLE;
        #ifndef REDFIS_SINGLE_THREAD
        xTaskCreatePinnedToCore(Diag_Cyclic, "DiagCore", 2048u, NULL, 3, &taskHandle,1);
        #endif
        callback = cb;
        retVal = DIAG_OK;
    }
    return retVal;
}

Diag_ReturnType Diag_DeInit(void)
{
    Diag_ReturnType retVal = DIAG_ERR;
    if (DIAG_IDLE == state)
    {
        #ifndef REDFIS_SINGLE_THREAD
        vTaskDelete(taskHandle);
        #endif
        Kwp_DeInit();
        callback = NULL;
        retVal = DIAG_OK;
    }
    return retVal;
}
