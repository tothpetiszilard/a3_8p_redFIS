#include "vwtp_cfg.h"
#include "dashapp.h"
#include "navapp.h"
#include "kwp.h"

#if ((1 == CONFIG_VWTP_DASH_TX_ID_ALTERNATIVE) && ( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE))
    #error "RNS-E and Alternative can not be selected in the same time!"
#elif (1 == CONFIG_VWTP_DASH_TX_ID_ALTERNATIVE)
VwTp_ChannelType vwtp_channels[2] =
#elif( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE)
VwTp_ChannelType vwtp_channels[3] =
#endif
{
    {
        .cfg = 
        {
            .mode = VWTP_NONDIAG,
            #if ((1 == CONFIG_VWTP_DASH_TX_ID_ALTERNATIVE) && ( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE))
                #error "RNS-E and Alternative can not be selected in the same time!"
            #elif( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE)
            .rxId = 0x6c1u,
            .txId = 0x6c0u,
            #elif (1 == CONFIG_VWTP_DASH_TX_ID_ALTERNATIVE)
            .rxId = 0x6c3u,
            .txId = 0x6c2u,
            #endif
            .blockSize = 0x0Fu,
            .ackTimeout = 0x8Au,
            .ips = 0x4Au,
            .txConfirmation = DashApp_TxConfirmation,
            .rxIndication = DashApp_Receive,
        }

    },
    {
        .cfg = 
        {
            .mode = VWTP_DIAG,
            .rxId = 0x0u,
            .txId = 0x200u,
            .blockSize = 0x0Fu,
            .ackTimeout = 0x8Au,
            .ips = 0x4Au,
            .txConfirmation = Kwp_TxConfirmation,
            .rxIndication = Kwp_Receive,
        }
#if ((1 == CONFIG_VWTP_DASH_TX_ID_ALTERNATIVE) && ( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE))
    #error "RNS-E and Alternative can not be selected in the same time!"
#elif( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE)
    },
    {
        .cfg = 
        {
            .mode = VWTP_NONDIAG,
            #if ((1 == CONFIG_VWTP_DASH_TX_ID_ALTERNATIVE) && ( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE))
                #error "RNS-E and Alternative can not be selected in the same time!"
            #elif( 1 == CONFIG_VWTP_DASH_TX_ID_NAVIGATION_RNSE)
            .rxId = 0x6c2u,
            .txId = 0x6c3u,
            #endif
            .blockSize = 0x0Fu,
            .ackTimeout = 0x8Au,
            .ips = 0x4Au,
            .txConfirmation = NavApp_TxConfirmation,
            .rxIndication = NavApp_Receive,
        }
#endif
    }
};