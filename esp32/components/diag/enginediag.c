/* Emulation of Tester which communicates to Engine controller */

#include "enginediag.h"
#include "kwp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

typedef struct
{
    uint32_t timestamp;
    uint8_t data[3u];
}EngineDiag_ChannelType;

static EngineDiag_ChannelType channels[ENGINEDIAG_CH_MAX];

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

static void EngineDiag_HandleDid(uint8_t * buffer, uint8_t actualDid);

Diag_ReturnType EngineDiag_Init(void)
{
    return Diag_Init(0x01u, EngineDiag_HandleDid);
}


Diag_ReturnType EngineDiag_GetChData(const EngineDiag_ChannelIdType ch, uint8_t * dataPtr, uint32_t timeout)
{
    Diag_ReturnType retVal = DIAG_ERR;
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
            retVal = DIAG_OK;
            xTaskResumeAll(); // End of critical section, interrupts enabled
        }
        else 
        {
            //Data is too old or never received
            if (DIAG_OK == Diag_ReqDid(ChIdxToDid[ch]))
            {
                retVal = DIAG_PENDING;
            }
        }
    }
    return retVal;
}

static void EngineDiag_HandleDid(uint8_t * buffer, uint8_t actualDid)
{
    uint32_t timestamp = 0u;
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
                    timestamp = xTaskGetTickCount();
                    vTaskSuspendAll(); // Critical section, interrupts enabled
                    for (b=0; b < 3u; b++)
                    {
                        // Copy 3 bytes of data from buffer with offset
                        channels[ch].data[b] = buffer[(offset * 3u)+b];
                    }
                    channels[ch].timestamp = timestamp;
                    xTaskResumeAll(); // End of critical section, interrupts enabled
                }
            }
        }
    }
}

