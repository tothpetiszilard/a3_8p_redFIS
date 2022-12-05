#include "enginediag.h"
#include "kwp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum
{
    ENGINEDIAG_IDLE,
    ENGINEDIAG_REQ,
    ENGINEDIAG_READ,
    ENGINEDIAG_READY
}EngineDiag_State;

typedef struct
{
    uint32_t timestamp;
    uint8_t data[3u];
}EngineDiag_ChannelType;

static EngineDiag_ChannelType channels[ENGINEDIAG_CH_MAX];
static EngineDiag_State state = ENGINEDIAG_IDLE;
static uint8_t actualDid = 0;
static TaskHandle_t taskHandle;


const uint8_t ChIdxToDid[ENGINEDIAG_CH_MAX] = 
{
    [ENGINEDIAG_CH_COOLANTTEMP1] = 1u,
    [ENGINEDIAG_CH_DURMI1] = 1u,
    [ENGINEDIAG_CH_QMI1] = 1u,
    [ENGINEDIAG_CH_ENGINESPEED1] = 1u,
    [ENGINEDIAG_CH_FUELTEMP] = 7u,
    [ENGINEDIAG_CH_IATTEMP7] = 7u,
    [ENGINEDIAG_CH_COOLANTTEMP7] = 7u,
    [ENGINEDIAG_CH_OILTEMP] = 29u,
    [ENGINEDIAG_CH_ENGINETEMP62] = 62u,
    [ENGINEDIAG_CH_COOLERTEMP62] = 62u,
    [ENGINEDIAG_CH_AMBITEMP62] = 62u,
    [ENGINEDIAG_CH_IATTEMP62] = 62u,
    [ENGINEDIAG_CH_EGTEMP67] = 67u,
    [ENGINEDIAG_CH_EGTEMP74] = 74u,
    [ENGINEDIAG_CH_LAMBDA74] = 74u,
};
const uint8_t ChIdxToDidOffset[ENGINEDIAG_CH_MAX] = 
{
    [ENGINEDIAG_CH_COOLANTTEMP1] = 3u,
    [ENGINEDIAG_CH_DURMI1] = 2u,
    [ENGINEDIAG_CH_QMI1] = 1u,
    [ENGINEDIAG_CH_ENGINESPEED1] = 0u,
    [ENGINEDIAG_CH_FUELTEMP] = 0u,
    [ENGINEDIAG_CH_IATTEMP7] = 2u,
    [ENGINEDIAG_CH_COOLANTTEMP7] = 3u,
    [ENGINEDIAG_CH_OILTEMP] = 0u,
    [ENGINEDIAG_CH_ENGINETEMP62] = 0u,
    [ENGINEDIAG_CH_COOLERTEMP62] = 1u,
    [ENGINEDIAG_CH_AMBITEMP62] = 2u,
    [ENGINEDIAG_CH_IATTEMP62] = 3u,
    [ENGINEDIAG_CH_EGTEMP67] = 0u,
    [ENGINEDIAG_CH_EGTEMP74] = 1u,
    [ENGINEDIAG_CH_LAMBDA74] = 2u
};

static void EngineDiag_Cyclic(void *pvParameters);
static void EngineDiag_HandleDid(uint8_t * buffer);

void EngineDiag_Init(void)
{
    vTaskDelay(210u / portTICK_PERIOD_MS);
    Kwp_Init(0x01u);
    state = ENGINEDIAG_IDLE;
    xTaskCreatePinnedToCore(EngineDiag_Cyclic, "EngineDiag", 2048u, NULL, 4, &taskHandle,1);
}

EngineDiag_ReturnType EngineDiag_GetChData(const EngineDiag_ChannelIdType ch, uint8_t * dataPtr, uint32_t timeout)
{
    EngineDiag_ReturnType retVal = ENGINEDIAG_ERR;
    uint32_t sysTime = 0;
    uint8_t i = 0;
    if (ch < ENGINEDIAG_CH_MAX)
    {
        sysTime = xTaskGetTickCount();
        if ( channels[ch].timestamp > (sysTime - (timeout/ portTICK_PERIOD_MS)))
        {
            // Data is not too old
            vTaskSuspendAll(); // Critical section, interrupts enabled
            for (i=0; i<3u; i++)
            {
                dataPtr[i] = channels[ch].data[i];
            }
            retVal = ENGINEDIAG_OK;
            xTaskResumeAll(); // End of critical section, interrupts enabled
        }
        else 
        {
            //Data is too old or never received
            if (ENGINEDIAG_IDLE == state)
            {
                vTaskSuspendAll(); // Critical section, interrupts enabled
                actualDid = ChIdxToDid[ch];
                state = ENGINEDIAG_REQ;
                retVal = ENGINEDIAG_PENDING;
                xTaskResumeAll(); // End of critical section, interrupts enabled
            }
        }
    }
    return retVal;
}

static void EngineDiag_Cyclic(void *pvParameters)
{
    static uint8_t errCnt = 0;
    uint8_t didBuffer[4u*3u];
    while(1)
    {
        vTaskDelay(50u / portTICK_PERIOD_MS);
        switch(state)
        {
            case ENGINEDIAG_IDLE:
            break;
            case ENGINEDIAG_REQ:
            if (KWP_OK == Kwp_RequestData(actualDid))
            {
                state = ENGINEDIAG_READ;
            }
            break;
            case ENGINEDIAG_READ:
            if (KWP_OK == Kwp_GetDataFromECU(didBuffer))
            {
                EngineDiag_HandleDid(didBuffer);
                state = ENGINEDIAG_IDLE;
            }
            else 
            {
                if (errCnt > 10)
                {
                    errCnt = 0;
                    state = ENGINEDIAG_REQ; // re-request
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
        vTaskDelay(50u / portTICK_PERIOD_MS);
    }
}

static void EngineDiag_HandleDid(uint8_t * buffer)
{
    uint32_t timestamp;
    uint8_t ch,offset,b; 
    for (ch = 0;ch<ENGINEDIAG_CH_MAX;ch++)
    {
        // Search for a channel with this DID
        if (actualDid == ChIdxToDid[ch])
        {
            // a channel found
            for (offset=0; offset < 4u; offset++)
            {
                // Search for a matching MBW from all the 4 MWBs we received
                if (offset == ChIdxToDidOffset[ch])
                {
                    vTaskSuspendAll(); // Critical section, interrupts enabled
                    for (b=0; b < 3u; b++)
                    {
                        // Copy 3 bytes of data from buffer with offset
                        channels[ch].data[b] = buffer[(offset * 3u)+b];
                    }
                    timestamp = xTaskGetTickCount();
                    channels[ch].timestamp = timestamp;
                    xTaskResumeAll(); // End of critical section, interrupts enabled
                }
            }
        }
    }
}

