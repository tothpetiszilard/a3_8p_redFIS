#include "vwtp_cfg.h"
#include "stddef.h"

VwTp_ChannelType vwtp_channels[1] =
{
    {
        .cfg = 
        {
            .rxId = 0x6c1u,
            .txId = 0x6c0u,
            .blockSize = 0x0Fu,
            .ackTimeout = 0x8Au,
            .ips = 0x4Au,
            .txConfirmation = NULL,
            .rxIndication = NULL,
        }

    }
};