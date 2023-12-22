/* A3 8P Red FIS project 2023 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "canWrapper.h"
#include "vwtp.h"
#include "dashapp.h"
#include "navapp.h"
#include "dis.h"
#include "enginediag.h"
#include "kwp.h"
#ifdef REDFIS_SINGLE_THREAD
#include "esp_task_wdt.h"
#endif

void app_main()
{
    #ifdef REDFIS_SINGLE_THREAD
    static uint64_t last10, last50, last100, last300;
    uint64_t sysTime;
    #endif
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0 : printf("Wakeup caused by external signal using RTC_IO\n"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : printf("Wakeup caused by external signal using RTC_CNTL\n"); break;
        case ESP_SLEEP_WAKEUP_TIMER : printf("Wakeup caused by timer\n"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : printf("Wakeup caused by touchpad\n"); break;
        case ESP_SLEEP_WAKEUP_ULP : printf("Wakeup caused by ULP program\n"); break;
        default : printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
    }

    Can_Init();
    VwTp_Init();
    DashApp_Init();
    NavApp_Init();
    //EngineDiag_Init();
    Dis_Init();
    #ifdef REDFIS_SINGLE_THREAD
    esp_task_wdt_add(NULL);
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

 