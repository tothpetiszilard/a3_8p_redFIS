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
        .labelCnt = 3,
        .labels = pageLabels[0],
        .dataCnt = 3,
        .data = pageData[0],
        .diagCnt = 3,
        .diagChs = pageDiag[0],
    }
};

static void Dis_CreateStrings(DashApp_ContentType * out, const DashApp_ContentType * const row, uint8_t * data);
static void HandleDisplay(const DisPageType * pagePtr);
static StalkButtons_Type buttons = STALKBUTTONS_NOEVENT;
static uint8_t actualPage = 0;
static uint8_t actualRow = 0;
static DashApp_ContentType dspBuffer;
static uint8_t diagBuffer[3u];
#if (1 == CONFIG_DIS_STALKBUTTON_RESET_EXIT_ENTER)
static uint8_t exited = 0;
#endif

void Dis_Init(void)
{
    
    #if (1 == CONFIG_BENCH_TEST_MODE)
    DashDiag_Init();
    #else
    EngineDiag_Init();
    #endif
    #ifndef REDFIS_SINGLE_THREAD
    vTaskDelay(160u / portTICK_PERIOD_MS);
    
    xTaskCreatePinnedToCore(Dis_Cyclic, "Dis", 2048u, NULL, 2, &disTaskHandle,1);
    #endif
}

void Dis_Cyclic(void *pvParameters)
{
    static uint8_t wasRoutingActive = 0;
    uint8_t isRoutingActive = 0;
    const DisPageType * pagePtr = NULL;
    DashApp_ReturnType dashStatus;
    #ifndef REDFIS_SINGLE_THREAD
    while(1)
    #endif
    {
        buttons = StalkButtons_Get();
        dashStatus = DashApp_GetStatus();
        if (DASHAPP_ERR != dashStatus)
        {
            if ((DASHAPP_PAUSE == dashStatus) && (0 != isRoutingActive))
            {
                isRoutingActive = 0;
                if (isRoutingActive != wasRoutingActive)
                {
                    if (NAVAPP_OK == NavApp_Pause())
                    {
                        wasRoutingActive = isRoutingActive;
                    }
                }
                
            }
            if ((STALKBUTTONS_UP == buttons) && ((1u + (sizeof(pages)/sizeof(pages[0])) > actualPage)))
            {
                actualPage++;
                actualRow = 0;
            }
            else if ((STALKBUTTONS_DOWN == buttons) && (0 < actualPage))
            {
                actualPage--;
                actualRow = 0;
            }
            #if (1 == CONFIG_DIS_STALKBUTTON_RESET_EXIT_ENTER)
            else if (STALKBUTTONS_RESET == buttons)
            {
                if (0 == exited)
                {
                    if (DASHAPP_OK == DashApp_Exit())
                    {
                        exited = 1;
                        actualRow = 0;
                    }
                    
                }
                else 
                {
                    if (DASHAPP_OK == DashApp_Enter())
                    {
                        exited = 0;
                    }
                }
                
            }
            #endif
            else
            {
                // Don't change the page 
            }
        }
        else 
        {
            // Dashboard is not available, report it to Nav also
            (void)NavApp_Pause();
        }
        if (actualPage < (sizeof(pages)/sizeof(pages[0])))
        {
            isRoutingActive = 0;
            if (wasRoutingActive != isRoutingActive)
            {
                if (NAVAPP_OK == NavApp_Pause())
                {
                    (void)DashApp_ClearScreen();
                    wasRoutingActive = isRoutingActive;
                }
            }
            else
            {
                pagePtr = &pages[actualPage];
                HandleDisplay(pagePtr);
            }
        }
        else if (DASHAPP_OK == dashStatus)
        {
            isRoutingActive = 1;
            if (wasRoutingActive != isRoutingActive)
            {
                if (DASHAPP_OK == DashApp_ClearScreen())
                {
                    #ifndef REDFIS_SINGLE_THREAD
                    vTaskDelay(300 / portTICK_PERIOD_MS);
                    #endif
                    (void)NavApp_Continue();
                    wasRoutingActive = isRoutingActive;
                }
            }
        }
        #ifndef REDFIS_SINGLE_THREAD
        vTaskDelay(300 / portTICK_PERIOD_MS);
        #endif
    }
}


static void HandleDisplay(const DisPageType * pagePtr)
{
    static DisDspTaskType dspTask = DIS_DISPLAY_C;
    DashApp_ReturnType status;
    status = DashApp_GetStatus();
    if (DASHAPP_OK == status)
    {
        /* Display is ready */
        switch(dspTask)
        {
            case DIS_DISPLAY_C:
            if (actualRow < pagePtr->labelCnt)
            {
                // Send constant labels
                if (DASHAPP_OK == DashApp_Print((DashApp_ContentType *)&pagePtr->labels[actualRow]))
                {
                    actualRow++;
                }
            }
            else
            {
                actualRow = 0;
                dspTask = DIS_DISPLAY_V;
            }
            break;
            case DIS_DISPLAY_V:
            if (actualRow < (pagePtr->dataCnt))
            {
                // Create strings from KWP data
                #if (1 == CONFIG_BENCH_TEST_MODE)
                if (DIAG_OK == DashDiag_GetChData(pagePtr->diagChs[actualRow].diagCh, diagBuffer, pagePtr->diagChs[actualRow].timeout))
                #else
                if (DIAG_OK == EngineDiag_GetChData(pagePtr->diagChs[actualRow].diagCh, diagBuffer, pagePtr->diagChs[actualRow].timeout))
                #endif
                {
                    Dis_CreateStrings(&dspBuffer,&pages[actualPage].data[actualRow],diagBuffer);
                    // Send values
                    if (DASHAPP_OK == DashApp_Print(&dspBuffer))
                    {
                        actualRow++;
                    }
                }
            }
            else
            {
                actualRow = 0;
                dspTask = DIS_DISPLAY_C;
            }
            break;
            default:
            break;
        }
    }
    else if (DASHAPP_ERR == status)
    {
        dspTask = DIS_DISPLAY_C;
        actualRow = 0;
        /* Display is not available */
    }
    else
    {
        /* A command is in progress */
    }
}

static void Dis_CreateStrings(DashApp_ContentType * out, const DashApp_ContentType * const row, uint8_t * data)
{
    out->ft = row->ft;
    out->posX = row->posX;
    out->posY = row->posY;
    out->mode = row->mode;
    out->len = Dis_DecodeFrame(out->string,data);
}
