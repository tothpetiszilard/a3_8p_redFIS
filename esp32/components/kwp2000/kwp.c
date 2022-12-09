#include "kwp.h"
#include "kwp_cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define KWP_SID_SESSION (0x10u) // Start diagnostic session
#define KWP_SID_RID     (0x31u) // Start routine by local identifier
#define KWP_SID_RDID    (0x21u) // Read data by local identifier
#define KWP_SID_RECUID  (0x1Au) // Read ECU identification
#define KWP_SID_POSRESP (0x40u)
#define KWP_SID_NEGRESP (0x7Fu)
#define KWP_SID_RCRRP   (0x78u) // Request correctly received response pending

typedef enum
{
    KWP_IDLE,
    KWP_INPROGRESS,
    KWP_WAITRESULT,
    KWP_ERROR
} Kwp_StatusType;

typedef enum
{
    KWP_CONNECT = 0,
    KWP_INIT = 1,
    KWP_SESSION = 0x10u,
    KWP_ECUID =   0x1Au,
    KWP_ROUTINE = 0x31u,
    KWP_READDID = 0x21u,
    KWP_READY =   0x81u,
    KWP_CLOSE =   0xFFu
} Kwp_StageType;

static Kwp_StatusType commandStatus = KWP_IDLE;
static Kwp_StageType  diagStage = KWP_INIT;
static TaskHandle_t   KwpTaskHdl = NULL;
static uint8_t        didBuffer[12u];
static uint8_t        dataId = 1u;
static uint8_t        ecuId = 1;
static uint8_t        retry = 0;

static void Kwp_Cyclic(void *pvParameters);
static void Kwp_StartSession(uint8_t sessionId);
static void Kwp_ReadEcuId(uint8_t idOption);
static void Kwp_StartRoutine(uint8_t rid, uint16_t rEntOpt);
static void Kwp_ReadData(uint8_t did);

void Kwp_Init(uint8_t ecu)
{
    ecuId = ecu;
    retry = 0;
    diagStage = KWP_CONNECT;
    xTaskCreatePinnedToCore(Kwp_Cyclic, "Kwp2000", 2048, NULL, 4, &KwpTaskHdl,1);
}

Kwp_ReturnType Kwp_RequestData(uint8_t did)
{
    Kwp_ReturnType retVal = KWP_ERR;
    if ((KWP_READY == diagStage) && (KWP_IDLE == commandStatus))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        dataId = did;
        diagStage = KWP_READDID;
        retVal = KWP_OK;
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    return retVal;
}

Kwp_ReturnType Kwp_GetDataFromECU(uint8_t * const dataPtr)
{
    Kwp_ReturnType retVal = KWP_ERR;
    uint8_t tmp;
    if ((KWP_READY == diagStage) && (KWP_IDLE == commandStatus))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        for(tmp=0;tmp<sizeof(didBuffer);tmp++)
        {
            dataPtr[tmp] = didBuffer[tmp]; // copy data
        }
        retVal = KWP_OK;
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    return retVal;
}

static void Kwp_Cyclic(void *pvParameters)
{
    static uint8_t timeout = 0;
    while(1)
    {
        vTaskDelay(50u / portTICK_PERIOD_MS);
        if (KWP_IDLE == commandStatus)
        {
            switch(diagStage)
            {
                case KWP_CONNECT:
                if (KWP_OK == Kwp_ConnectTp(ecuId))
                {
                    commandStatus = KWP_INPROGRESS;
                }
                else 
                {
                    if (retry > 10u)
                    {
                        retry = 0;
                        diagStage = KWP_ERROR;
                    }
                    else 
                    {
                        retry++;
                        vTaskDelay(500u / portTICK_PERIOD_MS);
                    }
                }
                break;
                case KWP_INIT:
                Kwp_StartSession(0x89u);
                break;
                case KWP_SESSION:
                Kwp_ReadEcuId(0x9Bu);
                break;
                case KWP_ECUID:
                Kwp_StartRoutine(0xB8u, 0x0000u);
                break;
                case KWP_ROUTINE:
                diagStage = KWP_READY;
                break;
                case KWP_READDID:
                Kwp_ReadData(dataId);
                break;
                case KWP_CLOSE:
                Kwp_DisconnectTp();
                vTaskDelay(1000u / portTICK_PERIOD_MS);
                retry = 0;
                diagStage = KWP_CONNECT;
                break;
                default:
                break;
            }
        }
        else if (KWP_INPROGRESS == commandStatus)
        {
            if (KWP_TIMEOUT_1SEC <= timeout)
            {
                if (KWP_CONNECT == diagStage)
                {
                    // Handle no response to connect timeout
                    commandStatus = KWP_IDLE;
                }
                else
                {
                    // No response from ECU
                    diagStage = KWP_CLOSE;
                    commandStatus = KWP_IDLE;
                }
                timeout=0u;
            }
            else 
            {
                timeout++;
            }
        }
        else if (KWP_WAITRESULT == commandStatus)
        {
            if (KWP_TIMEOUT_5SECS <= timeout)
            {
                // (Rx) timeout occured while waiting for response
                timeout=0;
                diagStage = KWP_CLOSE;
                commandStatus = KWP_IDLE;
            }
            else 
            {
                timeout++;
            }
        }
    }
}

void Kwp_Receive(uint8_t * dataPtr,uint16_t len)
{
    uint8_t i = 0;
    if (KWP_WAITRESULT == commandStatus)
    {
        if  (1u <= dataPtr[1])
        {
            vTaskSuspendAll(); // Critical section, interrupts enabled
            if (((KWP_SID_POSRESP + diagStage) == dataPtr[2]))
            {
                // Positive response
                if ((KWP_READDID == diagStage) && (dataId == dataPtr[3u]) && (sizeof(didBuffer) <= (len-4u)))
                {
                    
                    for (i=0;i<(sizeof(didBuffer) );i++)
                    {
                        didBuffer[i] = dataPtr[4u+i];
                    }
                    diagStage = KWP_READY;
                    
                }
                commandStatus = KWP_IDLE;
            }
            else if ((KWP_SID_NEGRESP == dataPtr[2u])&& (2u < dataPtr[1u]))
            {
                if ((diagStage == dataPtr[3u]) && (KWP_SID_RCRRP == dataPtr[4u]))
                {
                    // Response pending, stay in KWP_WAITRESULT
                }
                else
                {
                    // Negative response
                    commandStatus = KWP_ERROR;
                }
            }
            else 
            {
                // Corrupt TP frame or unexpected response
                diagStage = KWP_CLOSE;
                commandStatus = KWP_IDLE;
            }
            xTaskResumeAll(); // End of critical section, interrupts enabled
        }
    }
}

void Kwp_TxConfirmation(uint8_t result)
{
    if ((KWP_TP_ERR != result) && (KWP_INPROGRESS == commandStatus))
    {
        if (KWP_CONNECT == diagStage)
        {
            diagStage = KWP_INIT;
            commandStatus = KWP_IDLE;
        }
        else
        {
            commandStatus = KWP_WAITRESULT;
        }
    }
    else if (KWP_TP_ERR == result)
    {
        commandStatus = KWP_IDLE;
        retry = 0;
        diagStage = KWP_CONNECT;
    }
    else 
    {
        // Nothing to do
    }
}


static void Kwp_StartSession(uint8_t sessionId)
{
    uint8_t msg[4];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x02; // Length
    msg[2] = KWP_SID_SESSION; // SID
    msg[3] = sessionId;
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        diagStage = KWP_SESSION;
    }
}

static void Kwp_ReadEcuId(uint8_t idOption)
{
    uint8_t msg[4];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x02; // Length
    msg[2] = KWP_SID_RECUID; // SID
    msg[3] = idOption;
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        diagStage = KWP_ECUID;
    }
}

static void Kwp_StartRoutine(uint8_t rid, uint16_t rEntOpt)
{
    uint8_t msg[6];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x04; // Length
    msg[2] = KWP_SID_RID; // SID
    msg[3] = rid; // routine local id
    msg[4] = (uint8_t)rEntOpt; // RoutineEntryOption
    msg[5] = (uint8_t)((uint16_t)rEntOpt >> (uint16_t)8u); // RoutineEntryOption
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        diagStage = KWP_ROUTINE;
    }
}

static void Kwp_ReadData(uint8_t did)
{
    uint8_t msg[4];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x02; // Length
    msg[2] = KWP_SID_RDID; // SID
    msg[3] = did; // data local id
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        commandStatus = KWP_INPROGRESS;
        diagStage = KWP_READDID;
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
}

