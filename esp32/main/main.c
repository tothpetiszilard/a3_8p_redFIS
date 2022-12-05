/* A3 8P Red FIS project 2022 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "canWrapper.h"
#include "vwtp.h"
#include "dashapp.h"
#include "dis.h"
#include "enginediag.h"

void app_main()
{
    Can_Init();
    VwTp_Init();
    DashApp_Init();
    EngineDiag_Init();
    Dis_Init();
    while (1) 
    {
        vTaskDelete(NULL);
    }
}

 