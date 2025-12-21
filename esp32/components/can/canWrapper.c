#include "driver/gpio.h"
#ifndef USE_OLD_TWAI_DRIVER
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#else
#include "driver/twai.h"
#endif
#include "canWrapper.h"
#include "canWrapper_Cfg.h"

#include "freertos/FreeRTOS.h"

#ifndef USE_OLD_TWAI_DRIVER
static twai_node_handle_t nodeHdl = NULL;
volatile uint8_t can_txBuffer[8u];
static bool canTxBusy = false;
static bool Can_Receive(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx);
static bool Can_TxConfirmation(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx);
#else
static TaskHandle_t CanTaskHdl = NULL;
#endif

void Can_Init(void)
{
    #ifndef USE_OLD_TWAI_DRIVER
    twai_onchip_node_config_t nodeCfg = 
    {
        .io_cfg = {
            .tx = GPIO_NUM_21,
            .rx = GPIO_NUM_32,
            .quanta_clk_out = GPIO_NUM_NC,
            .bus_off_indicator = GPIO_NUM_NC,
        },
        .clk_src = 0,
        .bit_timing = {
            .bitrate = 500000u, /* 500kbps for cluster CAN */
            .sp_permill = 700,
            .ssp_permill = 800,
        },
        .fail_retry_cnt = -1, /* Hardware retry limit: -1 for re-trans forever */
        .tx_queue_depth = 10u,
        .intr_priority = 0,
        .flags = {
            .enable_self_test = false,     /* Default: Self-test disabled */
            .enable_loopback = false,      /* Default: Loopback disabled */
            .enable_listen_only = false,   /* Default: Listen-only disabled */
            .no_receive_rtr = 0, /* Don't receive remote frames */
        }
    };
    #else
    twai_general_config_t gcfg = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_32, TWAI_MODE_NORMAL);
    twai_timing_config_t tcfg = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t fcfg;
    gcfg.tx_queue_len = 10;
    gcfg.rx_queue_len = 15;
    gcfg.intr_flags |= ESP_INTR_FLAG_IRAM;
    #endif
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
    #ifndef USE_OLD_TWAI_DRIVER
    twai_new_node_onchip(&nodeCfg, &nodeHdl);
    twai_event_callbacks_t user_cbs = 
    {
        .on_rx_done = Can_Receive,
        .on_tx_done = Can_TxConfirmation
    };
    twai_node_register_event_callbacks(nodeHdl, &user_cbs, NULL);
    twai_mask_filter_config_t dual_config = twai_make_dual_filter(0x155u, 0x62Au, 0x200u, 0x5c7u, false); // id1, mask1, id2, mask2, no extend ID
    twai_node_config_mask_filter(nodeHdl, 0, &dual_config);
    twai_node_enable(nodeHdl);
    #else
    fcfg.acceptance_code = (0x155u << 21u) | (0x200u << 5u);
    fcfg.acceptance_mask = 0xC55FB8FFu;
    fcfg.single_filter = false;
    twai_driver_install(&gcfg, &tcfg, &fcfg);
    twai_start();
    #endif
    #ifndef REDFIS_SINGLE_THREAD
        #ifdef USE_OLD_TWAI_DRIVER
    xTaskCreatePinnedToCore(Can_Receive, "CanRx", 2048, NULL, 6, &CanTaskHdl,1);
        #endif
    #endif
    #if (1 == CONFIG_BENCH_TEST_MODE)
    CAN_IGN_RXINDICATION(7u); // Ignition is always active on the test-bench
    #else
    CAN_IGN_RXINDICATION(0u); // Go to sleep if no message received for ~30 seconds
    #endif
}

#ifndef USE_OLD_TWAI_DRIVER
static bool Can_TxConfirmation(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx)
{
    canTxBusy = false;
    return false;
}
static bool Can_Receive(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    uint8_t recv_buff[8u];
    twai_frame_t rx_frame = 
    {
        .buffer = recv_buff,
        .buffer_len = sizeof(recv_buff),
    };
    if (ESP_OK == twai_node_receive_from_isr(handle, &rx_frame)) 
    {
        // receive ok, do something here
        if (CAN_STALK_RXID == rx_frame.header.id)
        {
                CAN_STALK_RXINDICATION(rx_frame.buffer[1u]);
        }
        else if (CAN_IGNITION_RXID == rx_frame.header.id)
        {
            CAN_IGN_RXINDICATION(rx_frame.buffer[0u]);
        }
        else 
        {
            CANTP_RXINDICATION(rx_frame.header.id,rx_frame.header.dlc,rx_frame.buffer);
        }
    }
    return false;
}
#else
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
#endif

Can_ReturnType Can_Write(uint16_t id,uint8_t len,uint8_t * dataPtr)
{
    #ifndef USE_OLD_TWAI_DRIVER
    esp_err_t result;
    Can_ReturnType retVal = CAN_ERR;
    while (false != canTxBusy)
    {
        taskYIELD();
    }
    twai_frame_t tx_msg = {
        .header.id = id,          // Message ID
        .header.ide = false,      // Use 29-bit extended ID format
        .header.rtr = false,      // Non-remote frame
        .header.fdf = false,      // Non-FD frame
        .buffer = (uint8_t *)can_txBuffer,        // Pointer to data to transmit
        .buffer_len = len,        // Length of data to transmit
        .header.dlc = len
    };
    for (uint8_t i=0; i < len; i++)
    {
        can_txBuffer[i] = dataPtr[i];
    }
    result = twai_node_transmit(nodeHdl, &tx_msg, 5 / portTICK_PERIOD_MS);
    if (ESP_OK == result)
    {
        canTxBusy = true;
        /*result = twai_node_transmit_wait_all_done(nodeHdl,5 / portTICK_PERIOD_MS);
        if (ESP_OK == result)*/
        {
            retVal = CAN_OK;
        }
    }
    return retVal;
    #else
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
    #endif
}

void Can_DeInit(void)
{
    #ifndef USE_OLD_TWAI_DRIVER
    twai_node_disable(nodeHdl);
    twai_node_delete(nodeHdl);
    #else
    twai_stop();
    twai_driver_uninstall();
    #endif
}
