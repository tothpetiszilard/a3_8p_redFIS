/* Emulation of Navigation which communicates to Dash */

#include "dashapp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dashapp_cfg.h"

#define DASHAPP_KEY_OFF (2)
#define DASHAPP_KL15_ON (1)

typedef enum
{
    DASHAPP_INIT=0,
    DASHAPP_PWRSTATE=1,
    DASHAPP_IDREQ=2,
    DASHAPP_PAGEREQ=3,
    DASHAPP_GETSTATUS=4,
    DASHAPP_SEND2F=5,
    DASHAPP_PREWRITE=6,
    DASHAPP_SHOW=7,
    DASHAPP_WRITE=8,
    DASHAPP_READY=9,
    DASHAPP_CLEAR = 10,
    DASHAPP_WAIT,
    DASHAPP_SUSPEND,
    DASHAPP_SHUTDOWN
} DashApp_StateType;

static uint8_t ignitionState = DASHAPP_KL15_ON;
static DashApp_StateType appState = DASHAPP_INIT;
static uint8_t waitForAck = 0;
static uint8_t retryCnt = 0;
static TaskHandle_t DashAppTaskHdl = NULL;
static char DashApp_FixASCII(char c);

static DashApp_ContentType dspContent;

// Rx
static void DashApp_HandlePwrState(uint8_t val);
static void DashApp_HandleDashID(uint8_t * data);
static void DashApp_HandleReqStatus(uint8_t val);
// Tx
static void DashApp_SendPwrReport(void);
static void DashApp_GetDashID(void);
static void DashApp_ReqMfaPage(uint8_t pageId);
static void DashApp_ReqArea(uint8_t startX,uint8_t startY,uint8_t endX, uint8_t endY, uint8_t response);
static void DashApp_Write(void);
static void DashApp_Send2FResp(void);
static void DashApp_Show(void);
static void DashApp_InitDisplay(void);
static void DashApp_Clear(void);

void DashApp_Init(void)
{
    appState = DASHAPP_INIT;
    waitForAck = 0;
    retryCnt = 0;
    #ifndef REDFIS_SINGLE_THREAD
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreatePinnedToCore(DashApp_Cyclic, "DashApp", 2560u, NULL, 4, &DashAppTaskHdl,1);
    #endif
}

void DashApp_Cyclic(void *pvParameters)
{
    static uint8_t initTimeout = 0;
    #ifndef REDFIS_SINGLE_THREAD
    while(1u)
    #endif
    {
        if (0 == waitForAck)
        {
            if (0 != DASHAPP_GETIGNITION())
            {
                switch(appState)
                {
                    case DASHAPP_INIT:
                    if (initTimeout < 120u)
                    {
                        initTimeout++;
                    }
                    else 
                    {
                        initTimeout = 0;
                        appState = DASHAPP_PWRSTATE;
                    }
                    break;
                    case DASHAPP_PWRSTATE:
                    DashApp_SendPwrReport();
                    break;
                    case DASHAPP_IDREQ:
                    DashApp_GetDashID();
                    break;
                    case DASHAPP_PAGEREQ:
                    DashApp_ReqMfaPage(0u);
                    break;
                    case DASHAPP_GETSTATUS:
                    DashApp_ReqArea(0u,27u,64u,48u,1u);
                    break;
                    case DASHAPP_SEND2F:
                    DashApp_Send2FResp();
                    break;
                    case DASHAPP_PREWRITE:
                    DashApp_InitDisplay();
                    break;
                    case DASHAPP_WRITE:
                    DashApp_Write();
                    break;
                    case DASHAPP_SHOW:
                    DashApp_Show();// Show page 
                    break;
                    case DASHAPP_CLEAR:
                    DashApp_Clear();// Delete page 
                    break;
                    default:
                    break;
                }
            }
            else
            {
                if (DASHAPP_INIT != appState)
                {
                    DASHAPP_DISCONNECT();
                    appState = DASHAPP_INIT;
                    waitForAck = 0;
                }
            }
        }
        #ifndef REDFIS_SINGLE_THREAD
        vTaskDelay(30 / portTICK_PERIOD_MS);
        #endif
    }
}

DashApp_ReturnType DashApp_GetStatus(void)
{
    DashApp_ReturnType retVal = DASHAPP_ERR;
    if (DASHAPP_READY == appState)
    {
        retVal = DASHAPP_OK;
    }
    else if (DASHAPP_WRITE == appState)
    {
        retVal = DASHAPP_BUSY;
    }
    else if (DASHAPP_SUSPEND == appState)
    {
        retVal = DASHAPP_PAUSE;
    }
    else 
    {
        /* Return not ok */
    }
    return retVal;
}

DashApp_ReturnType DashApp_Enter(void)
{
    DashApp_ReturnType retVal = DASHAPP_ERR;
    if (DASHAPP_SUSPEND == appState)
    {
        appState = DASHAPP_GETSTATUS;
        retVal = DASHAPP_OK;
    }
    else 
    {
        /* Return not ok */
    }
    return retVal;
}

DashApp_ReturnType DashApp_Exit(void)
{
    DashApp_ReturnType retVal = DASHAPP_ERR;
    if (DASHAPP_READY == appState)
    {
        appState = DASHAPP_CLEAR;
        retVal = DASHAPP_OK;
    }
    else if (DASHAPP_WRITE == appState)
    {
        retVal = DASHAPP_BUSY;
    }
    else 
    {
        /* Return not ok */
    }
    return retVal;
}

DashApp_ReturnType DashApp_Print(const DashApp_ContentType * const content)
{
    DashApp_ReturnType retVal = DASHAPP_ERR;
    uint8_t tmp;
    if ((DASHAPP_READY == appState) && (NULL != content))
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        dspContent.ft = content->ft;
        dspContent.len = content->len;
        dspContent.mode = content->mode;
        dspContent.posX = content->posX;
        dspContent.posY = content->posY;
        for (tmp=0; tmp < dspContent.len; tmp++)
        {
            dspContent.string[tmp] = content->string[tmp];
        }
        
        if (dspContent.mode == DASHAPP_ADD)
        {
            appState = DASHAPP_WRITE;
        }
        else 
        {
            appState = DASHAPP_PREWRITE;
        }
        retVal = DASHAPP_OK;
        xTaskResumeAll(); // End of critical section, interrupts enabled
    }
    return retVal;
}

DashApp_ReturnType DashApp_ClearScreen(void)
{
    DashApp_ReturnType retVal = DASHAPP_ERR;
    if (DASHAPP_READY == appState)
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        dspContent.len = 0;
        appState = DASHAPP_PREWRITE;
        xTaskResumeAll(); // End of critical section, interrupts enabled
        retVal = DASHAPP_OK;
    }
    return retVal;
}

void DashApp_TxConfirmation(uint8_t result)
{
    if (VWTP_OK == result)
    {
        waitForAck = 0;
    }
    else if (VWTP_ERR == result)
    {
        appState = DASHAPP_INIT;
    }
    else 
    {
        // Nothing to do
    }
}

void DashApp_Receive(uint8_t * dataPtr,uint16_t len)
{
    switch(dataPtr[0])
    {
        case DASHAPP_CMD_PWRSTATE:
        // request 0 from dash, response is 1
        DashApp_HandlePwrState(dataPtr[1]);
        break;
        case DASHAPP_CMD_ID:
        // ID, response to 8
        DashApp_HandleDashID(&dataPtr[1]);
        break;
        case DASHAPP_CMD_PAGERESP:
        // response to 20
        appState = DASHAPP_GETSTATUS;
        break;
        case DASHAPP_CMD_2E_REQ:
        // request from dash, 2F is the response
        appState = DASHAPP_SEND2F;
        break;
        case DASHAPP_CMD_REQSTATUS:
        DashApp_HandleReqStatus(dataPtr[1]);
        // response to 52 (read?)
        break;
        case DASHAPP_CMD_ERR:
        DASHAPP_DISCONNECT();
        appState = DASHAPP_INIT;
        waitForAck = 0;
        break;
        default:
        break;
    }
}

static void DashApp_InitDisplay(void)
{
    if (VWTP_OK == DASHAPP_SENDTP((uint8_t*)DashApp_DspInit,sizeof(DashApp_DspInit)))
    {
        waitForAck = 1u;
        appState = DASHAPP_SHOW;
    }
}

static void DashApp_ReqArea(uint8_t startX,uint8_t startY,uint8_t endX, uint8_t endY, uint8_t response)
{
    uint8_t msg[7];
    msg[0] = DASHAPP_CMD_REQAREA;
    msg[1] = 0x05u; // length, 5 bytes
    msg[2] = 0x02u;
    if (0u != response)
    {
        msg[2] |= 0x80u; //response required
    }
    msg[3] = startX;
    msg[4] = startY;
    msg[5] = endX;
    msg[6] = endY;
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = DASHAPP_WAIT;
    }
}

static void DashApp_SendPwrReport(void)
{
    uint8_t msg[3];
    msg[0] = DASHAPP_CMD_PWRREPORT;
    msg[1] = ignitionState;
    msg[2] = 0; // ??? 
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        if (DASHAPP_KEY_OFF != ignitionState)
        {
            if (DASHAPP_SHUTDOWN == appState)
            {
                #ifndef REDFIS_SINGLE_THREAD
                xTaskCreatePinnedToCore(DashApp_Cyclic, "DashApp", 2048, NULL, 5, &DashAppTaskHdl,1);
                #endif
            }
            appState = DASHAPP_IDREQ;
        }
        else 
        {
            //appState = DASHAPP_SHUTDOWN;
            //vTaskDelete(DashAppTaskHdl);
            //DASHAPP_DISCONNECT();
        }
        
    }
    else 
    {
        if(retryCnt > 3u)
        {
            appState = DASHAPP_INIT;
            retryCnt = 0;
        }
        else 
        {
            retryCnt++;
        }
    }
}

static void DashApp_ReqMfaPage(uint8_t pageId)
{
    uint8_t msg[4];
    msg[0] = DASHAPP_CMD_PAGEREQ;
    msg[1] = 0x3B;
    msg[2] = 0xA0;
    msg[3] = pageId;
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = DASHAPP_WAIT;
    }
}

static void DashApp_GetDashID(void)
{
    uint8_t msg[1];
    msg[0] = DASHAPP_CMD_IDREQ;
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = DASHAPP_WAIT;
    }
}

static void DashApp_Send2FResp(void)
{
    uint8_t msg[1];
    msg[0] = DASHAPP_CMD_2F_RESP;
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = DASHAPP_GETSTATUS;
    }
}

static void DashApp_Write(void)
{
    uint8_t msg[dspContent.len + 5];
    uint8_t i = 0;
    if (0u < dspContent.len)
    {
        vTaskSuspendAll(); // Critical section, interrupts enabled
        msg[0] = DASHAPP_CMD_WRITE;
        msg[1] = dspContent.len + 3; // string length + 3
        msg[2] = (uint8_t)dspContent.ft; // params: font
        msg[3] = dspContent.posX;
        msg[4] = dspContent.posY;
        for(i=0;i< dspContent.len;i++)
        {
            msg[5+i] = DashApp_FixASCII(dspContent.string[i]);
        }
        xTaskResumeAll(); // End of critical section, interrupts enabled
        if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
        {
            vTaskSuspendAll(); // Critical section, interrupts enabled
            waitForAck = 1u;
            appState = DASHAPP_READY;
            xTaskResumeAll(); // End of critical section, interrupts enabled
        }
    }
    else
    {
        appState = DASHAPP_READY;
    }
}

static void DashApp_Show(void)
{
    uint8_t msg[1];
    msg[0] = DASHAPP_CMD_SHOW;
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = DASHAPP_READY;
    }
}

static void DashApp_Clear(void)
{
    uint8_t msg[1];
    msg[0] = DASHAPP_CMD_CLEAR;
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = DASHAPP_SUSPEND;
    }
}

static void DashApp_HandlePwrState(uint8_t val)
{
    ignitionState = val;
    appState = DASHAPP_PWRSTATE;
}

static void DashApp_HandleReqStatus(uint8_t val)
{
    if (0xC0 == val)
    {
        // Request rejected, parameter error
        DASHAPP_DISCONNECT();
        appState = DASHAPP_INIT;
        waitForAck = 0;
    }
    else if (0x05 == val)
    {
        // Request OK, pending, wait for 2E
        appState = DASHAPP_WAIT;
    }
    else if ((0x04 == val) || (0x84 == val))
    {
        // Request OK, busy, wait for 85
        appState = DASHAPP_SUSPEND;
    }
    else if (0x85 == val)
    {
        // Data transfer
        appState = DASHAPP_PREWRITE;
    }
    else
    {
        // Unknown error
    }

}

static void DashApp_HandleDashID(uint8_t * data)
{
    // TODO: Check part id?
    appState = DASHAPP_PAGEREQ;
}

static char DashApp_FixASCII(char c)
{
    char retVal;
    switch(c)
    {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        retVal = (c-(char)96);
        break;
        case ' ':
        retVal = (char)0x65;
        break;
        default:
        retVal = c;
        break;
    }
    return retVal;
}
