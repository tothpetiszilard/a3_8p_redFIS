/* Partial implementation of Keyword Protokoll */

#include "kwp.h"
#include "kwp_cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sysStates.h"
#include "dashdiag.h"

#define KWP_SID_SESSION   (0x10u) // Start diagnostic session
#define KWP_SID_SECURITY  (0x27u) // Security access
#define KWP_SID_RID       (0x31u) // Start routine by local identifier
#define KWP_SID_DOWNLOAD  (0x34u) // Request download
#define KWP_SID_UPLOAD    (0x35u) // Request upload
#define KWP_SID_TRANSFER  (0x36u) // Transfer Data
#define KWP_SID_TRANSEXIT (0x37u) // Request transfer exit
#define KWP_SID_RDID      (0x21u) // Read data by local identifier
#define KWP_SID_RECUID    (0x1Au) // Read ECU identification
#define KWP_SID_TPRESENT  (0x3Eu) // Tester present
#define KWP_SID_POSRESP   (0x40u)
#define KWP_SID_NEGRESP   (0x7Fu)
#define KWP_SID_RCRRP     (0x78u) // Request correctly received response pending

typedef enum
{
    KWP_IDLE,
    KWP_INPROGRESS,
    KWP_WAITRESULT,
    KWP_ERROR
} Kwp_StatusType;

typedef enum
{
    KWP_STAGE_CONNECT = 0,
    KWP_STAGE_INIT = 1,
    KWP_STAGE_SESSION89,
    KWP_STAGE_ECUID,
    KWP_STAGE_SEED,
    KWP_STAGE_KEY,
    KWP_STAGE_SESSION85,
    KWP_STAGE_FBLSEED,
    KWP_STAGE_FBLKEY,
    KWP_STAGE_ROUTINE,
    KWP_STAGE_READDID,
    KWP_STAGE_READY,
    KWP_STAGE_UPLOAD,
    KWP_STAGE_DOWNLOAD,
    KWP_STAGE_TRANSFER,
    KWP_STAGE_TRANSFEREXIT,
    KWP_STAGE_CLOSE,
} Kwp_StageType;

typedef enum
{
    KWP_NOSID = 0,
    KWP_SESSION = KWP_SID_SESSION,
    KWP_ECUID =   KWP_SID_RECUID,
    KWP_ROUTINE = KWP_SID_RID,
    KWP_READDID = KWP_SID_RDID,
    KWP_SEEDKEY = KWP_SID_SECURITY,
    KWP_UPLOAD = KWP_SID_UPLOAD,
    KWP_DOWNLOAD = KWP_SID_DOWNLOAD,
    KWP_TRANSFER = KWP_SID_TRANSFER,
    KWP_TRANSEXIT = KWP_SID_TRANSEXIT
} Kwp_ServiceId;

static Kwp_StatusType commandStatus = KWP_IDLE;
static Kwp_StageType  diagStage = KWP_STAGE_CONNECT;
static TaskHandle_t   KwpTaskHdl = NULL;
static Kwp_ServiceId  currentSid = KWP_NOSID;
static uint8_t        didBuffer[12u];
static uint8_t        dataId = 1u;
static uint8_t        ecuId = 1;
static uint8_t        retry = 0;
static uint8_t        active = 0;
static uint32_t       seed = 0;
static uint32_t       key = 0;
static uint32_t       address = 0xF0000u;

static void Kwp_StartSession(uint8_t sessionId);
static void Kwp_ReadEcuId(uint8_t idOption);
static void Kwp_StartRoutine(uint8_t rid, uint16_t rEntOpt);
static void Kwp_ReadData(uint8_t did);
static void Kwp_SecurityAccess(uint8_t accessMode, uint32_t keyOption);
static void Kwp_ReadMemByAddr(uint32_t addr, uint8_t size);
static void Kwp_ReqUpload(uint32_t addr, uint32_t size);
static void Kwp_ReqDownload(uint32_t addr, uint32_t size);
static void Kwp_TransferData(void);
static void Kwp_TransferExit(void);

Kwp_ReturnType Kwp_Init(uint8_t ecu)
{
    Kwp_ReturnType retVal = KWP_ERR;
    if (0 == active)
    {
        ecuId = ecu;
        retry = 0;
        active = 0;
        diagStage = KWP_STAGE_CONNECT;
        #ifndef REDFIS_SINGLE_THREAD
        xTaskCreatePinnedToCore(Kwp_Cyclic, "Kwp2000", 2048, NULL, 4, &KwpTaskHdl,1);
        #endif
        retVal = KWP_OK;
    }
    return retVal;
}

Kwp_ReturnType Kwp_DeInit(void)
{
    Kwp_ReturnType retVal = KWP_ERR;
    if ((1 == active) && (KWP_IDLE == commandStatus))
    {
        diagStage = KWP_STAGE_CLOSE;
        vTaskDelay(150u / portTICK_PERIOD_MS); // 150 ms delay to close connection from cyclic
        if (0 == active)
        {
            // Shutdown was successful
            vTaskDelete(KwpTaskHdl);
            retVal = KWP_OK;
        }
    }
    return retVal;
}

Kwp_ReturnType Kwp_GetConnectionState(void)
{
    Kwp_ReturnType retVal = KWP_ERR;
    if (0 != active)
    {
        retVal = KWP_OK;
    }
    return retVal;
}

Kwp_ReturnType Kwp_RequestData(uint8_t did)
{
    Kwp_ReturnType retVal = KWP_ERR;
    if ((KWP_STAGE_READY == diagStage) && (KWP_IDLE == commandStatus))
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
    if ((KWP_STAGE_READY == diagStage) && (KWP_IDLE == commandStatus))
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

void Kwp_Cyclic(void *pvParameters)
{
    static uint8_t timeout = 0;
    #ifndef REDFIS_SINGLE_THREAD
    while(1)
    #endif
    {
        #ifndef REDFIS_SINGLE_THREAD
        vTaskDelay(50u / portTICK_PERIOD_MS);
        #endif
        if (KWP_IDLE == commandStatus)
        {
            timeout=0u;
            if (0 != SysStates_GetIgnition())
            {
                switch(diagStage)
                {
                    case KWP_STAGE_CONNECT:
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
                            #ifndef REDFIS_SINGLE_THREAD
                            vTaskDelay(500u / portTICK_PERIOD_MS);
                            #endif
                        }
                    }
                    break;
                    case KWP_STAGE_INIT:
                    if (KWP_SID_SESSION != currentSid)
                    {
                        Kwp_StartSession(0x89u);
                        diagStage = KWP_STAGE_SESSION89;
                    }
                    else 
                    {
                        diagStage = KWP_STAGE_SESSION85;
                    }
                    break;
                    case KWP_STAGE_SESSION89:
                    Kwp_ReadEcuId(0x9Bu);
                    //Kwp_StartSession(0x85u);
                    diagStage = KWP_STAGE_ECUID;
                    break;
                    case KWP_STAGE_ECUID:
                    //Kwp_StartRoutine(0xE1u, 0x0000u);
                    key = 0;
                    Kwp_SecurityAccess(3u, key);
                    diagStage = KWP_STAGE_SEED;
                    break;
                    case KWP_STAGE_SEED:
                    if ((0 == key) && (0 != seed))
                    {
                        key = seed + 0x3625U;
                        Kwp_SecurityAccess(4u, key);
                        seed = 0;
                        diagStage = KWP_STAGE_KEY;
                    }
                    break;
                    case KWP_STAGE_KEY:
                    Kwp_StartSession(0x85u);
                    //Kwp_ReadMemByAddr(0x0ffe10u,4);
                    //Kwp_ReqUpload(0x0ffe10u,4u);
                    diagStage = KWP_STAGE_SESSION85;
                    vTaskDelay(500u / portTICK_PERIOD_MS);
                    break;
                    case KWP_STAGE_SESSION85:
                    Kwp_SecurityAccess(1u, 0);
                    diagStage = KWP_STAGE_FBLSEED;
                    break;
                    case KWP_STAGE_FBLSEED:
                    key = DashDiag_FblKey(seed);
                    Kwp_SecurityAccess(2u, key);
                    diagStage = KWP_STAGE_FBLKEY;
                    break;
                    case KWP_STAGE_FBLKEY:
                    //Kwp_ReqUpload(0x2000u,16u);
                    //diagStage = KWP_STAGE_UPLOAD;
                    //Kwp_ReadMemByAddr(0x0ffe10u,4);
                    //Kwp_ReqDownload(0xF0000u,0x7FFFu + 1u);
                    Kwp_ReqDownload(0x80000u,0xFFFFu + 1u);
                    diagStage = KWP_STAGE_DOWNLOAD;
                    break;
                    case KWP_STAGE_UPLOAD:
                    Kwp_TransferData();
                    diagStage = KWP_STAGE_TRANSFER;
                    break;
                    case KWP_STAGE_TRANSFER:
                    Kwp_TransferExit();
                    diagStage = KWP_STAGE_TRANSFEREXIT;
                    break;
                    case KWP_STAGE_ROUTINE:
                    diagStage = KWP_STAGE_READY;
                    break;
                    case KWP_STAGE_READDID:
                    Kwp_ReadData(dataId);
                    break;
                    case KWP_STAGE_CLOSE:
                    Kwp_DisconnectTp();
                    active = 0;
                    #ifndef REDFIS_SINGLE_THREAD
                    vTaskDelay(1000u / portTICK_PERIOD_MS);
                    #endif
                    retry = 0;
                    diagStage = KWP_STAGE_CONNECT;
                    break;
                    default:
                    break;
                }
            }
            else 
            {
                if (diagStage != KWP_STAGE_CONNECT)
                {
                    Kwp_DisconnectTp();
                    retry = 0;
                    diagStage = KWP_STAGE_CONNECT;
                    active = 0;
                } 
            }
        }
        else if (KWP_INPROGRESS == commandStatus)
        {
            if (KWP_TIMEOUT_1SEC <= timeout)
            {
                if (KWP_STAGE_CONNECT == diagStage)
                {
                    // Handle no response to connect timeout
                    commandStatus = KWP_IDLE;
                }
                else
                {
                    // No response from ECU
                    diagStage = KWP_STAGE_CLOSE;
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
                diagStage = KWP_STAGE_CLOSE;
                commandStatus = KWP_IDLE;
            }
            else 
            {
                timeout++;
            }
        }
        else 
        {
            // Error state
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
            if (((KWP_SID_POSRESP + currentSid) == dataPtr[2]))
            {
                // Positive response
                if ((KWP_STAGE_READDID == diagStage) && (KWP_READDID == currentSid) && (dataId == dataPtr[3u]) && (sizeof(didBuffer) <= (len-4u)))
                {
                    
                    for (i=0;i<(sizeof(didBuffer) );i++)
                    {
                        didBuffer[i] = dataPtr[4u+i];
                    }
                    diagStage = KWP_STAGE_READY;
                    currentSid = KWP_NOSID;
                    
                }
                else if (((KWP_STAGE_SEED == diagStage) || (KWP_STAGE_FBLSEED == diagStage))  && (KWP_SEEDKEY == currentSid) && (4u <= (len-4u)))
                {
                    seed = ((dataPtr[4u] << 24u) | (dataPtr[5u] << 16u) | (dataPtr[6u] << 8u) | dataPtr[7u]);
                }
                else if ((KWP_STAGE_KEY == diagStage) && (KWP_SEEDKEY == currentSid) && (2u <= (len-4u)))
                {
                    /*if ((dataPtr[3u] == 4u) && (0x34u == dataPtr[4u]))
                    {
                        // access granted
                        //diagStage = KWP_STAGE_KEY;
                    }*/
                }
                else if ((KWP_STAGE_UPLOAD == diagStage) && (KWP_SID_UPLOAD == currentSid))
                {

                }
                else if ((KWP_STAGE_DOWNLOAD == diagStage) && (KWP_SID_DOWNLOAD == currentSid))
                {
                    // Max block size
                }
                else if ((KWP_STAGE_SESSION85 == diagStage) && (KWP_SID_SESSION == currentSid))
                {
                    //vTaskDelay(50u / portTICK_PERIOD_MS);
                }
                commandStatus = KWP_IDLE;
            }
            else if ((KWP_SID_NEGRESP == dataPtr[2u])&& (2u < dataPtr[1u]))
            {
                if ((currentSid == dataPtr[3u]) && (KWP_SID_RCRRP == dataPtr[4u]))
                {
                    // Response pending, stay in KWP_WAITRESULT
                }
                else if ((currentSid == dataPtr[3u]) && (0x90u == dataPtr[4u]))
                {
                    // Restart, Bootloader does not support standby session
                    // Continue hacking...
                    diagStage = KWP_STAGE_SESSION85;
                }
                else
                {
                    // Negative response
                    commandStatus = KWP_ERROR;
                }
            }
            else if ((KWP_SID_POSRESP + KWP_SID_TPRESENT) != dataPtr[2])
            {
                // Corrupt TP frame or unexpected response
                diagStage = KWP_STAGE_CLOSE;
                commandStatus = KWP_IDLE;
            }
            else 
            {
              // Nothing to do
            }
            xTaskResumeAll(); // End of critical section, interrupts enabled
        }
    }
}

void Kwp_TxConfirmation(uint8_t result)
{
    if ((KWP_TP_ERR != result) && (KWP_INPROGRESS == commandStatus))
    {
        if (KWP_STAGE_CONNECT == diagStage)
        {
            diagStage = KWP_STAGE_INIT;
            commandStatus = KWP_IDLE;
            active = 1;
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
        diagStage = KWP_STAGE_CONNECT;
        active = 0;
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
        currentSid = KWP_SID_SESSION;
    }
}

static void Kwp_ReadMemByAddr(uint32_t addr, uint8_t size)
{
    uint8_t msg[7];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x05; // Length
    msg[2] = 0x23; // SID
    msg[3] = (addr & 0xFF0000u) >> 16u;
    msg[4] = (addr & 0xFF00u) >> 8u;
    msg[5] = (addr & 0xFFu);
    msg[6] = size;
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        currentSid = 0x23;
    }
}

static void Kwp_ReqUpload(uint32_t addr, uint32_t size)
{
    uint8_t msg[10];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x08; // Length
    msg[2] = KWP_SID_UPLOAD; // SID
    msg[3] = (addr & 0xFF0000u) >> 16u;
    msg[4] = (addr & 0xFF00u) >> 8u;
    msg[5] = (addr & 0xFFu);
    msg[6] = 0;
    msg[7] = (size & 0xFF0000u) >> 16u;
    msg[8] = (size & 0xFF00u) >> 8u;
    msg[9] = (size & 0xFFu);
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        currentSid = KWP_SID_UPLOAD;
    }
}

static void Kwp_ReqDownload(uint32_t addr, uint32_t size)
{
    uint8_t msg[10];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x08; // Length
    msg[2] = KWP_SID_DOWNLOAD; // SID
    msg[3] = (addr & 0xFF0000u) >> 16u;
    msg[4] = (addr & 0xFF00u) >> 8u;
    msg[5] = (addr & 0xFFu);
    msg[6] = 0x0;
    msg[7] = (size & 0xFF0000u) >> 16u;
    msg[8] = (size & 0xFF00u) >> 8u;
    msg[9] = (size & 0xFFu);
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        currentSid = KWP_SID_DOWNLOAD;
    }
}

static void Kwp_SecurityAccess(uint8_t accessMode, uint32_t keyOption)
{
    if ((0x01u == accessMode) || (0x03u == accessMode))
    {
        uint8_t msg[4];
        msg[0] = 0x00; // Format byte
        msg[1] = 0x02; // Length
        msg[2] = KWP_SID_SECURITY; // SID
        msg[3] = accessMode;
        (void)keyOption; // key is not used in this case
        if (KWP_TP_OK == Kwp_SendTp(msg))
        {
            commandStatus = KWP_INPROGRESS;
            currentSid = KWP_SEEDKEY;
        }
    }
    else if ((0x02u == accessMode) || (0x04u == accessMode))
    {
        uint8_t msg[8];
        msg[0] = 0x00; // Format byte
        msg[1] = 0x06; // Length
        msg[2] = KWP_SID_SECURITY; // SID
        msg[3] = accessMode;
        msg[4] = (keyOption & 0xFF000000ul) >> 24u;
        msg[5] = (keyOption & 0x00FF0000ul) >> 16u;
        msg[6] = (keyOption & 0x0000FF00ul) >> 8u;
        msg[7] = (keyOption & 0xFFu);
        if (KWP_TP_OK == Kwp_SendTp(msg))
        {
            commandStatus = KWP_INPROGRESS;
            currentSid = KWP_SEEDKEY;
        }
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
        currentSid = KWP_ECUID;
    }
}

static void Kwp_TransferData(void)
{
    uint8_t msg[3];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x01; // Length
    msg[2] = KWP_SID_TRANSFER; // SID
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        currentSid = KWP_SID_TRANSFER;
    }
}

static void Kwp_TransferExit(void)
{
    uint8_t msg[3];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x01; // Length
    msg[2] = KWP_SID_TRANSEXIT; // SID
    if (KWP_TP_OK == Kwp_SendTp(msg))
    {
        commandStatus = KWP_INPROGRESS;
        currentSid = KWP_SID_TRANSFER;
    }
}

static void Kwp_StartRoutine(uint8_t rid, uint16_t rEntOpt)
{
    uint8_t msg[6];
    msg[0] = 0x00; // Format byte
    msg[1] = 0x04; // Length
    msg[2] = KWP_SID_RID; // SID
    msg[3] = rid; // routine local id
    msg[4] = (uint8_t)((uint16_t)rEntOpt >> (uint16_t)8u); // RoutineEntryOption
    msg[5] = (uint8_t)rEntOpt; // RoutineEntryOption
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

