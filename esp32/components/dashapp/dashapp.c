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
    DASHAPP_WRITE=7,
    DASHAPP_WRITE2=8,
    DASHAPP_SHOW=9,
    DASHAPP_WAIT
} DashApp_StateType;

static uint8_t ignitionState = DASHAPP_KEY_OFF;
static DashApp_StateType appState = DASHAPP_INIT;
static uint8_t waitForAck = 0;
static TaskHandle_t DashAppTaskHdl = NULL;

static void DashApp_Cyclic(void *pvParameters);
// Rx
static void DashApp_HandlePwrState(uint8_t val);
static void DashApp_HandleDashID(uint8_t * data);
static void DashApp_HandleReqStatus(uint8_t val);
// Tx
static void DashApp_SendPwrReport(void);
static void DashApp_GetDashID(void);
static void DashApp_ReqMfaPage(uint8_t pageId);
static void DashApp_ReqArea(uint8_t startX,uint8_t startY,uint8_t endX, uint8_t endY, uint8_t response);
static void DashApp_Print(uint8_t x,uint8_t y,char * string, uint8_t len);
static void DashApp_Send2FResp(void);
static void DashApp_Show(void);
static void DashApp_InitDisplay(void);
//static void DashApp_Clear(void);

void DashApp_Init(void)
{
    appState = DASHAPP_INIT;
    waitForAck = 0;
    xTaskCreate(DashApp_Cyclic, "DashApp", 2048, NULL, 5, &DashAppTaskHdl);
}

static void DashApp_Cyclic(void *pvParameters)
{
    while(1u)
    {
        if (0 == waitForAck)
        {
            switch(appState)
            {
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
                // Write demo data 
                DashApp_Print(0, 1, "A3 8P", 5);
                if (0 != waitForAck) appState = DASHAPP_WRITE2; // switch if transmit was OK
                break;
                case DASHAPP_WRITE2:
                // Write demo data 
                DashApp_Print(15, 14, "RED FIS", 7);
                if (0 != waitForAck) appState = DASHAPP_SHOW; // switch if transmit was OK
                break;
                case DASHAPP_SHOW:
                DashApp_Show();// Show page 
                break;
                default:
                break;
            }
        }
        vTaskDelay(20 * portTICK_PERIOD_MS);
    }
}

void DashApp_TxConfirmation(void)
{
    waitForAck = 0;
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
        default:
        break;
    }
}

static void DashApp_InitDisplay(void)
{
    if (VWTP_OK == DASHAPP_SENDTP((uint8_t*)DashApp_DspInit,sizeof(DashApp_DspInit)))
    {
        waitForAck = 1u;
        appState = DASHAPP_WRITE;
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
        appState = DASHAPP_IDREQ;
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

static void DashApp_Print(uint8_t x,uint8_t y,char * string,uint8_t len)
{
    uint8_t msg[len + 5];
    uint8_t i = 0;
    msg[0] = DASHAPP_CMD_WRITE;
    msg[1] = len+3; // string length + 3
    msg[2] = 0x02; // params: small font
    msg[3] = x;
    msg[4] = y;
    for(i=0;i< len;i++)
    {
        msg[5+i] = string[i];
    }
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
    }
}

static void DashApp_Show(void)
{
    uint8_t msg[1];
    msg[0] = DASHAPP_CMD_SHOW;
    if (VWTP_OK == DASHAPP_SENDTP(msg,sizeof(msg)))
    {
        waitForAck = 1u;
        appState = DASHAPP_WAIT;
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
        // back to DASHAPP_GETSTATUS ?
    }
    else if (0x05 == val)
    {
        // Request OK, pending, wait for 2E
        appState = DASHAPP_WAIT;
    }
    else if ((0x04 == val) || (0x84 == val))
    {
        // Request OK, busy, wait for 85
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
