#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "canWrapper.h"
#include "canWrapper_Cfg.h"

static TaskHandle_t CanTaskHdl = NULL;

void Can_Init(void)
{
    twai_general_config_t gcfg = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_26, GPIO_NUM_32, TWAI_MODE_NORMAL);
    twai_timing_config_t tcfg = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t fcfg;
    gcfg.tx_queue_len = 10;
    gcfg.rx_queue_len = 15;
    gcfg.intr_flags |= ESP_INTR_FLAG_IRAM;
    // Accepted IDs are: 35F (stalk buttons), 6c1 (dash), 201 (engine), 300 (engine)
    fcfg.acceptance_code = (0x35Fu << 21u) | (0x200u << 5u);
    fcfg.acceptance_mask = 0x001FB83Fu;
    fcfg.single_filter = false;
    twai_driver_install(&gcfg, &tcfg, &fcfg);
    twai_start();
    #ifndef REDFIS_SINGLE_THREAD
    xTaskCreatePinnedToCore(Can_Receive, "CanRx", 2048, NULL, 6, &CanTaskHdl,1);
    #endif
}

void Can_Receive(void *pvParameters)
{
    twai_message_t msg;
    esp_err_t result;
    #ifndef REDFIS_SINGLE_THREAD
    while(1)
    #endif
    {
        #ifndef REDFIS_SINGLE_THREAD
        result = twai_receive(&msg, portMAX_DELAY);
        #else
        result = twai_receive(&msg, 0u);
        #endif
        if(result == ESP_OK)
        {
            // CAN frame received
            if (CAN_RXID == msg.identifier)
            {
                CAN_RXINDICATION(msg.data[1u]);
            }
            else 
            {
                CANTP_RXINDICATION(msg.identifier,msg.data_length_code,msg.data);
            }
        }
    }
}

Can_ReturnType Can_Write(uint16_t id,uint8_t len,uint8_t * dataPtr)
{
    uint8_t i;
    twai_message_t msg;
    esp_err_t result;
    Can_ReturnType retVal = CAN_ERR;
    msg.identifier = id;
    msg.data_length_code = len;
    msg.rtr = 0;
    msg.extd = 0;
    msg.ss = 0;
    msg.self = 0;
    msg.dlc_non_comp = 0;
    msg.reserved = 0;
    for (i=0; i < len; i++)
    {
        msg.data[i] = dataPtr[i];
    }
    result = twai_transmit(&msg, 5 / portTICK_PERIOD_MS);
    if (ESP_OK == result)
    {
        retVal = CAN_OK;
    }
    return retVal;
}

void Can_DeInit(void)
{
    twai_stop();
    twai_driver_uninstall();
    
}
