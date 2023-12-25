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
    // Set up CAN ID filters
    // ---- First
    //01101011111 35F (stalk buttons)
    //10101110101 575 (ignition)
    // Code: 00101010101 155
    // Mask: 11000101010 62A
    // ---- Second
    //01000000001 201 (engine diag)
    //01000000111 207 (dash diag)
    //01100000000 300 (all diag)
    //11011000001 6c1 (dash)
    //11011000010 6c2 (navi, gw mode)
    //11011000011 6c3 (dash, alternative mode)
    // Code: 01000000000 200
    // Mask: 10111000111 5c7
    fcfg.acceptance_code = (0x155u << 21u) | (0x200u << 5u);
    fcfg.acceptance_mask = 0xC55FB8FFu;
    fcfg.single_filter = false;
    twai_driver_install(&gcfg, &tcfg, &fcfg);
    twai_start();
    #ifndef REDFIS_SINGLE_THREAD
    xTaskCreatePinnedToCore(Can_Receive, "CanRx", 2048, NULL, 6, &CanTaskHdl,1);
    #endif
    #if (1 == CONFIG_BENCH_TEST_MODE)
    CAN_IGN_RXINDICATION(7u); // Ignition is always active on the test-bench
    #else
    CAN_IGN_RXINDICATION(0u); // Go to sleep if no message received for ~30 seconds
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
            if (CAN_STALK_RXID == msg.identifier)
            {
                CAN_STALK_RXINDICATION(msg.data[1u]);
            }
            else if (CAN_IGNITION_RXID == msg.identifier)
            {
                CAN_IGN_RXINDICATION(msg.data[0u]);
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
