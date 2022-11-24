/* A3 8P Red FIS project 2022 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "canWrapper.h"
#include "vwtp.h"
#include "dashapp.h"

void app_main()
{
    Can_Init();
    VwTp_Init();
    DashApp_Init();
    while (1) 
    {
        vTaskDelay(6000 / portTICK_PERIOD_MS);
    }
}

 