/* Emulation of Tester which communicates to Dashboard */

#include "dashdiag.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



typedef struct
{
    uint32_t timestamp;
    uint8_t data[3u];
}DashDiag_ChannelType;

static DashDiag_ChannelType channels[DASHDIAG_CH_MAX];

const uint8_t ChIdxToDid[DASHDIAG_CH_MAX] = 
{
    [DASHDIAG_CH_VEHICLESPEED] = 1u,
    [DASHDIAG_CH_ENGINESPEED] = 1u,
    [DASHDIAG_CH_OILPRESSURE] = 1u,
    [DASHDIAG_CH_COOLANTTEMP] = 3u,
    [DASHDIAG_CH_FUELLEVEL1] = 26u,
    [DASHDIAG_CH_FUELLEVEL2] = 26u,
};
const uint8_t ChIdxToDidOffset[DASHDIAG_CH_MAX] = 
{
    [DASHDIAG_CH_VEHICLESPEED] = 0u,
    [DASHDIAG_CH_ENGINESPEED] = 1u,
    [DASHDIAG_CH_OILPRESSURE] = 2u,
    [DASHDIAG_CH_COOLANTTEMP] = 0u,
    [DASHDIAG_CH_FUELLEVEL1] = 1u,
    [DASHDIAG_CH_FUELLEVEL2] = 3u,
};

static void DashDiag_HandleDid(uint8_t * buffer, uint8_t actualDid);

Diag_ReturnType DashDiag_Init(void)
{
    return Diag_Init(0x07u, DashDiag_HandleDid);
}

Diag_ReturnType DashDiag_GetChData(const DashDiag_ChannelIdType ch, uint8_t * dataPtr, uint32_t timeout)
{
    Diag_ReturnType retVal = DIAG_ERR;
    uint32_t sysTime = 0;
    uint8_t i = 0;
    if (ch < DASHDIAG_CH_MAX)
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

static void DashDiag_HandleDid(uint8_t * buffer, uint8_t actualDid)
{
    uint32_t timestamp = 0u;
    uint8_t ch,offset,b; 
    for (ch = 0;ch<DASHDIAG_CH_MAX;ch++)
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

