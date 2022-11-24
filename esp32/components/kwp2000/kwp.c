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
    KWP_INIT = 0,
    KWP_SESSION = 0x10u,
    KWP_ECUID =   0x1Au,
    KWP_ROUTINE = 0x31u,
    KWP_READDID = 0x21u,
    KWP_READY =   0x81u
} Kwp_StageType;

static Kwp_StatusType commandStatus = KWP_IDLE;
static Kwp_StageType  diagStage = KWP_INIT;
static TaskHandle_t   KwpTaskHdl = NULL;
static uint8_t        didBuffer[12u];
static uint8_t        dataId = 1u;

static void Kwp_Cyclic(void *pvParameters);
static void Kwp_StartSession(uint8_t sessionId);
static void Kwp_ReadEcuId(uint8_t idOption);
static void Kwp_StartRoutine(uint8_t rid, uint16_t rEntOpt);
static void Kwp_ReadData(uint8_t did);

void Kwp_Init(uint8_t ecuId)
{
    if (KWP_OK == Kwp_ConnectTp(ecuId))
    {
        commandStatus = KWP_INPROGRESS;
        diagStage = KWP_INIT;
        xTaskCreate(Kwp_Cyclic, "Kwp2000", 2048, NULL, 5, &KwpTaskHdl);
    }
}

static void Kwp_Cyclic(void *pvParameters)
{
    while(1)
    {
        if (KWP_IDLE == commandStatus)
        {
            switch(diagStage)
            {
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
                //case KWP_READDID:
                Kwp_ReadData(dataId);
                break;
                default:
                break;
            }
        }
        vTaskDelay(100 * portTICK_PERIOD_MS);
    }
}

void Kwp_Receive(uint8_t * dataPtr,uint16_t len)
{
    uint8_t i = 0;
    if (KWP_WAITRESULT == commandStatus)
    {
        if ((0 == dataPtr[0]) && (1u <= dataPtr[1]))
        {
            if (((KWP_SID_POSRESP + diagStage) == dataPtr[2]))
            {
                // Positive response
                if ((KWP_READDID == diagStage) && (dataId == dataPtr[3]) && (sizeof(didBuffer) > (len-4u)))
                {
                    for (i=0;i<(len-4u);i++)
                    {
                        didBuffer[i] = dataPtr[4u+i];
                    }
                }
                commandStatus = KWP_IDLE;
            }
            else if ((KWP_SID_NEGRESP == dataPtr[2])&& (2u < dataPtr[1]))
            {
                if ((diagStage == dataPtr[2]) && (KWP_SID_RCRRP == dataPtr[3]))
                {
                    // Response pending, stay in KWP_WAITRESULT
                }
                else
                {
                    // Negative response
                    commandStatus = KWP_ERROR;
                }
            }
        }
    }
}

void Kwp_TxConfirmation(void)
{
    if (KWP_INPROGRESS == commandStatus)
    {
        if (KWP_INIT == diagStage)
        {
            commandStatus = KWP_IDLE;
        }
        else
        {
            commandStatus = KWP_WAITRESULT;
        }
    }
}


static void Kwp_StartSession(uint8_t sessionId)
{
    uint8_t msg[4];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x02; // Length
    msg[2] = KWP_SID_SESSION; // SID
    msg[3] = sessionId;
    if (KWP_OK == Kwp_SendTp(msg))
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
    if (KWP_OK == Kwp_SendTp(msg))
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
    if (KWP_OK == Kwp_SendTp(msg))
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
    if (KWP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        diagStage = KWP_READDID;
    }
}

