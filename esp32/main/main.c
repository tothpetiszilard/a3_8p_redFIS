/* A3 8P Red FIS project 2022 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "canWrapper.h"
#include "vwtp.h"
#include "dashapp.h"
#include "dis.h"
#include "enginediag.h"
#include "kwp.h"
#include "esp_task_wdt.h"

void app_main()
{
    #ifdef REDFIS_SINGLE_THREAD
    static uint64_t last10, last50, last100, last300;
    uint64_t sysTime;
    #endif
    Can_Init();
    VwTp_Init();
    DashApp_Init();
    EngineDiag_Init();
    Dis_Init();
    esp_task_wdt_add(NULL);
    #ifdef REDFIS_SINGLE_THREAD
    while(1)
    {
        sysTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        Can_Receive(NULL);
        esp_task_wdt_reset();
        if (sysTime >= last10 + 10u)
        {
            last10 = sysTime;
            VwTp_Cyclic(NULL);
        }
        if (sysTime >= last50 + 50u)
        {
            last50 = sysTime;
            Kwp_Cyclic(NULL);
            DashApp_Cyclic(NULL);
        }
        if (sysTime >= last100 + 100u)
        {
            last100 = sysTime;
            EngineDiag_Cyclic(NULL);
        }
        if (sysTime >= last300 + 300u)
        {
            last300 = sysTime;
            Dis_Cyclic(NULL);
        }
    }
    #else
    vTaskDelete(NULL);
    #endif
}

 