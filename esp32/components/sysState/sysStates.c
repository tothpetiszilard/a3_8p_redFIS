#include "sysStates.h"
//#include "stdio.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static uint8_t ignitionState_u8 = 0;
static TaskHandle_t ShutdownHookHdl = NULL;
static uint8_t shutdownStarted = 0;

static void ShutdownHook(void *pvParameters);

void SysStates_Receive(uint8_t data)
{
    ignitionState_u8 = (data & 0xFu);
    if ((0 != (ignitionState_u8 & 7u)) && (0 !=shutdownStarted))
    {
        vTaskDelete(ShutdownHookHdl);
        //printf("Shutdown cancelled\n");
        shutdownStarted = 0;
    }
    else if ((0 == (ignitionState_u8 & 7u)) && (0 == shutdownStarted))
    {
        xTaskCreatePinnedToCore(ShutdownHook, "ShutdownHook", 1024u, NULL, 6, &ShutdownHookHdl,1);
        shutdownStarted = 1;
    }
    else 
    {
        // Shutdown hook is already started
    }
}

static void ShutdownHook(void *pvParameters)
{
    //printf("Shutdown in 30 secs\n");
    vTaskDelay(30000 / portTICK_PERIOD_MS);
    //printf("Shutdown now\n");
    esp_sleep_enable_ext1_wakeup(0x100000000UL,ESP_EXT1_WAKEUP_ALL_LOW); // PIN 32 is a wakeup source
    vTaskDelay(100 / portTICK_PERIOD_MS);
    esp_deep_sleep_start();
}

uint8_t SysStates_GetIgnition(void)
{
    uint8_t retVal_u8 = 0;
    if (0 != (ignitionState_u8 & 7u))
    {
        retVal_u8 = 1;
    }
    else 
    {
        retVal_u8 = 0;
    }
    return retVal_u8;
}

