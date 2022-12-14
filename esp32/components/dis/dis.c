#include "dis.h"
#include "dis_cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "kwp.h"
#include "kwp_cfg.h"
#include "string.h"
#include "stalkButtons.h"

typedef enum
{
    DIS_PROCESS,
    DIS_DISPLAY_C,
    DIS_DISPLAY_V,
    
}DisDspTaskType;

static TaskHandle_t disTaskHandle;

static const DisPageType pages[1] = 
{
    {
        .rows_used = 3,
        .labels = pageLabels[0],
        .data = pageData[0],
        .diagChs = pageDiag[0],
    }
};

static void Dis_CreateStrings(DashApp_ContentType * out, const DashApp_ContentType * const row, uint8_t * data);
static void HandleDisplay(DisPageType * pagePtr);
static DisDspTaskType dspTask = DIS_DISPLAY_C;
static StalkButtons_Type buttons = STALKBUTTONS_NOEVENT;
static uint8_t actualPage = 0;
static DashApp_ContentType dspBuffer;
static uint8_t diagBuffer[3u];

void Dis_Init(void)
{
    dspTask = DIS_DISPLAY_C;
    #ifndef REDFIS_SINGLE_THREAD
    vTaskDelay(160u / portTICK_PERIOD_MS);
    
    xTaskCreatePinnedToCore(Dis_Cyclic, "Dis", 2048u, NULL, 2, &disTaskHandle,1);
    #endif
}

void Dis_Cyclic(void *pvParameters)
{
    DisPageType * pagePtr = NULL;
    #ifndef REDFIS_SINGLE_THREAD
    while(1)
    #endif
    {
        pagePtr = (DisPageType *)&pages[actualPage];
        buttons = StalkButtons_Get();
        HandleDisplay(pagePtr);
        #ifndef REDFIS_SINGLE_THREAD
        vTaskDelay(300 / portTICK_PERIOD_MS);
        #endif
    }
}


static void HandleDisplay(DisPageType * pagePtr)
{
    static uint8_t rowCnt = 0;
    switch(dspTask)
    {
        case DIS_DISPLAY_C:
        if (rowCnt < pagePtr->rows_used)
        {
            // Send constant labels
            if (DASHAPP_OK == DashApp_Print((DashApp_ContentType *)&pagePtr->labels[rowCnt]))
            {
                rowCnt++;
            }
        }
        else
        {
            rowCnt = 0;
            dspTask = DIS_PROCESS;
        }
        break;
        case DIS_PROCESS:
        if (rowCnt < pagePtr->rows_used)
        {

            // Create strings from the data
            if (ENGINEDIAG_OK == EngineDiag_GetChData(pagePtr->diagChs[rowCnt].diagCh, diagBuffer, pagePtr->diagChs[rowCnt].timeout))
            {
                Dis_CreateStrings(&dspBuffer,&pagePtr->data[rowCnt],diagBuffer);
                dspTask = DIS_DISPLAY_V;
            }
        }
        else
        {
            rowCnt = 0;
            dspTask = DIS_DISPLAY_C;
        }
        break;
        case DIS_DISPLAY_V:
        if (rowCnt < pagePtr->rows_used)
        {
            // Send values
            if (DASHAPP_OK == DashApp_Print(&dspBuffer))
            {
                rowCnt++;
                dspTask = DIS_PROCESS;
            }
        }
        else
        {
            rowCnt = 0;
            dspTask = DIS_DISPLAY_C;
        }
        break;
        default:
        break;
    }
}

static void Dis_CreateStrings(DashApp_ContentType * out, const DashApp_ContentType * const row, uint8_t * data)
{
    int16_t val_s16 = 0;
    /*static uint8_t cnt = 0;
    if (buttons == STALKBUTTONS_UP)
    {
        cnt++;
    }
    else if ((buttons == STALKBUTTONS_DOWN) && (cnt > 0))
    {
        cnt--;
    }
    else
    {
        //Keep the value
    }*/
    //out->len = Dis_DecodeFrame(out->string,data);

    //out->len = snprintf(out->string, sizeof(out->string),row->string ,data[0] );
    out->ft = row->ft;
    out->posX = row->posX;
    out->posY = row->posY;
    out->mode = row->mode;
    if (data[0] == 5)
    {
        val_s16 = data[1] * (data[2] - 100);
        val_s16 /= 10;
        out->len = snprintf(out->string, sizeof(out->string),row->string ,val_s16 );
    }
    else 
    {
        out->len = snprintf(out->string, sizeof(out->string),"---" );
    }
}
