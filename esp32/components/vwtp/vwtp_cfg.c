#include "vwtp_cfg.h"
#include "dashapp.h"
#include "kwp.h"


VwTp_ChannelType vwtp_channels[2] =
{
    {
        .cfg = 
        {
            .mode = VWTP_NONDIAG,
            .rxId = 0x6c1u,
            .txId = 0x6c0u,
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

    }
};