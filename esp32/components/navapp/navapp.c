/* Emulation of Dashboard which communicates to Navigation */

#include "navapp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "navapp_cfg.h"

#define NAVAPP_KEY_OFF (2)
#define NAVAPP_KL15_ON (1)

typedef enum
{
    NAVAPP_INIT=0,
    NAVAPP_PWRSTATE=1,
    NAVAPP_IDREQ=2,
    NAVAPP_PAGEREQ=3,
    NAVAPP_STATUS=4,
    NAVAPP_SEND2E=5,
    NAVAPP_PREWRITE=6,
    NAVAPP_SHOW=7,
    NAVAPP_WRITE=8,
    NAVAPP_READY=9,
    NAVAPP_CLEAR = 10,
    NAVAPP_WAIT,
    NAVAPP_SUSPEND,
    NAVAPP_SHUTDOWN
} NavApp_StateType;

static uint8_t ignitionState = NAVAPP_KL15_ON;
static NavApp_StateType appState = NAVAPP_INIT;
static uint8_t waitForAck = 0;
static uint8_t retryCnt = 0;
static uint8_t reqPageId = 0;
static uint8_t reqResp = 0xC0;
static TaskHandle_t NavAppTaskHdl = NULL;

// Rx
static void NavApp_HandlePwrReport(uint8_t val);
static void NavApp_HandleDashIDReq(void);
static void NavApp_HandleReqMfaPage(uint8_t * val);
static void NavApp_HandleReqArea(uint8_t * request);
static void NavApp_HandleWrite(void);
static void NavApp_HandleShow(void);
static void NavApp_HandleClear(void);
// Tx
static void NavApp_SendPwrState(void);
static void NavApp_SendDashID(void);
static void NavApp_ReqMfaPageResp(void);
static void NavApp_ReqAreaResponse(void);
static void NavApp_Send2E(void);


void NavApp_Init(void)
{
    appState = NAVAPP_INIT;
    waitForAck = 0;
    retryCnt = 0;
    #ifndef REDFIS_SINGLE_THREAD
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreatePinnedToCore(NavApp_Cyclic, "NavApp", 2560u, NULL, 4, &NavAppTaskHdl,1);
    #endif
}

void NavApp_Cyclic(void *pvParameters)
{
    static uint8_t initTimeout = 0;
    #ifndef REDFIS_SINGLE_THREAD
    while(1u)
    #endif
    {
        if (0 == waitForAck)
        {
            if (0 != NAVAPP_GETIGNITION())
            {
                switch(appState)
                {
                    case NAVAPP_INIT:
                    if (initTimeout < 120u)
                    {
                        initTimeout++;
                    }
                    else 
                    {
                        initTimeout = 0;
                        appState = NAVAPP_PWRSTATE;
                    }
                    break;
                    case NAVAPP_PWRSTATE:
                    NavApp_SendPwrState();
                    break;
                    case NAVAPP_IDREQ:
                    NavApp_SendDashID();
                    break;
                    case NAVAPP_PAGEREQ:
                    NavApp_ReqMfaPageResp();
                    break;
                    case NAVAPP_STATUS:
                    NavApp_ReqAreaResponse();
                    break;
                    case NAVAPP_SEND2E:
                    NavApp_Send2E();
                    break;
                    case NAVAPP_WRITE:
                    // TODO : Get buffer to route into?
                    break;
                    default:
                    break;
                }
            }
            else
            {
                if (NAVAPP_INIT != appState)
                {
                    NAVAPP_DISCONNECT();
                    appState = NAVAPP_INIT;
                    waitForAck = 0;
                }
            }
        }
        #ifndef REDFIS_SINGLE_THREAD
        vTaskDelay(30 / portTICK_PERIOD_MS);
        #endif
    }
}

NavApp_ReturnType NavApp_GetStatus(void)
{
    NavApp_ReturnType retVal = NAVAPP_ERR;
    if (NAVAPP_READY == appState)
    {
        retVal = NAVAPP_OK;
    }
    else if (NAVAPP_WRITE == appState)
    {
        retVal = NAVAPP_BUSY;
    }
    else if (NAVAPP_SUSPEND == appState)
    {
        retVal = NAVAPP_PAUSE;
    }
    else 
    {
        /* Return not ok */
    }
    return retVal;
}

void NavApp_TxConfirmation(uint8_t result)
{
    if (VWTP_OK == result)
    {
        waitForAck = 0;
    }
    else if (VWTP_ERR == result)
    {
        appState = NAVAPP_INIT;
    }
    else 
    {
        // Nothing to do
    }
}

void NavApp_Receive(uint8_t * dataPtr,uint16_t len)
{
    switch(dataPtr[0])
    {
        case NAVAPP_CMD_PWRREPORT:
        // request 1 from nav, response is 0
        NavApp_HandlePwrReport(dataPtr[1]);
        break;
        case NAVAPP_CMD_IDREQ:
        // ID request (8), respond with 9
        NavApp_HandleDashIDReq();
        break;
        case NAVAPP_CMD_PAGEREQ:
        NavApp_HandleReqMfaPage(&dataPtr[1]);
        break;
        case NAVAPP_CMD_2F_RESP:
        // response from nav, followed by data
        appState = NAVAPP_WRITE;
        break;
        case NAVAPP_CMD_REQAREA:
        NavApp_HandleReqArea(&dataPtr[1]);
        break;
        case NAVAPP_CMD_WRITE:
        // Data received to be shown, drop it for now...
        break;
        case NAVAPP_SHOW:
        case NAVAPP_CLEAR:
        // Show/Clear command received, do nothing for now
        break;
        case NAVAPP_CMD_ERR:
        NAVAPP_DISCONNECT();
        appState = NAVAPP_INIT;
        waitForAck = 0;
        break;
        default:
        break;
    }
}


static void NavApp_HandleReqArea(uint8_t * request)
{
    if (0 != (request[1] & 0x80))
    {
        if (NAVAPP_WRITE != appState)
        {
            reqResp = 0x05; // Preparation, wait for 2E
            appState = NAVAPP_STATUS;
        }
        else 
        {
            reqResp = 0x85; // Ready, clear to send
        }
    }
    else 
    {
        appState = NAVAPP_WAIT;
    }
}

static void NavApp_SendPwrState(void)
{
    uint8_t msg[3];
    msg[0] = NAVAPP_CMD_PWRSTATE;
    msg[1] = ignitionState;
    msg[2] = 0; // ??? 
    if (VWTP_OK == NAVAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        if (NAVAPP_KEY_OFF != ignitionState)
        {
            appState = NAVAPP_WAIT;
        }
        
    }
    else 
    {
        if(retryCnt > 3u)
        {
            appState = NAVAPP_INIT;
            retryCnt = 0;
        }
        else 
        {
            retryCnt++;
        }
    }
}

static void NavApp_ReqMfaPageResp()
{
    uint8_t msg[4];
    msg[0] = NAVAPP_CMD_PAGERESP;
    msg[1] = 0x3B;
    msg[2] = 0xA0;
    msg[3] = reqPageId;
    if (VWTP_OK == NAVAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = NAVAPP_WAIT;
    }
}

static void NavApp_SendDashID(void)
{
    uint8_t msg[12];
    msg[0] = NAVAPP_CMD_ID;
    msg[1] = 0x20;
    msg[2] = 0x0B;
    msg[3] = 0x50;
    msg[4] = 0x07;
    msg[5] = 0x29;
    msg[6] = 0x50;
    msg[7] = 0x30;
    msg[8] = 0x39;
    msg[9] = 0x00;
    msg[10] = 0x30;
    msg[11] = 0x00;
    if (VWTP_OK == NAVAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = NAVAPP_WAIT;
    }
}

static void NavApp_ReqAreaResponse()
{
    uint8_t msg[2];
    msg[0] = NAVAPP_CMD_REQSTATUS;
    msg[1] = reqResp;
    if (VWTP_OK == NAVAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        if (reqResp == 5)
        {
            appState = NAVAPP_SEND2E;
        }
        else
        {
            appState = NAVAPP_WAIT;
        }     
    }
}

static void NavApp_Send2E(void)
{
    uint8_t msg[1];
    msg[0] = NAVAPP_CMD_2E_REQ;
    if (VWTP_OK == NAVAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = NAVAPP_WAIT;
    }
}

static void NavApp_HandleWrite(void)
{
    
}

static void NavApp_Show(void)
{
    uint8_t msg[1];
    msg[0] = NAVAPP_CMD_SHOW;
    if (VWTP_OK == NAVAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = NAVAPP_READY;
    }
}

static void NavApp_Clear(void)
{
    uint8_t msg[1];
    msg[0] = NAVAPP_CMD_CLEAR;
    if (VWTP_OK == NAVAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = NAVAPP_SUSPEND;
    }
}

static void NavApp_HandlePwrReport(uint8_t val)
{
    ignitionState = val;
    appState = NAVAPP_PWRSTATE;
}

static void NavApp_HandleReqMfaPage(uint8_t * val)
{
    //Answer with 21?
    if ((val[0] == 0x3B) && (val[1] == 0xA0))
    {
        reqPageId = val[2]; // store pageID for the response
        appState = NAVAPP_PAGEREQ;
    }
}

static void NavApp_HandleDashIDReq(void)
{
    //Send part id
    appState = NAVAPP_IDREQ;
}

