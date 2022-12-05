#include "dis_cfg.h"
#include "dashapp.h"

const EngineDiag_ChannelIdType pageDiag[1][3] = 
{
    // Page0
    {
        // Row 0
        ENGINEDIAG_CH_IATTEMP7, // did 7
        // Row 1
        ENGINEDIAG_CH_OILTEMP, // did 29
        // Row 2
        ENGINEDIAG_CH_FUELTEMP // did7
    }
};

//data format to printf format string

const DashApp_ContentType pageData[1][3] = 
{
    {
    // Page 0
        {
        // Row 0
        .mode = DASHAPP_ADD,
        .posX = 24,
        .posY = 11,
        .string = "%- 3d C",
        .len = 4,
        .ft = DASHAPP_FONT_L,
        },
        {
        // Row 1
        .mode = DASHAPP_ADD,
        .posX = 24,
        .posY = 21,
        .string = "%- 3d C",
        .len = 4,
        .ft = DASHAPP_FONT_L,
        },
        {
        // Row 2
        .mode = DASHAPP_ADD,
        .posX = 24,
        .posY = 31,
        .string = "%- 3d C",
        .len = 4,
        .ft = DASHAPP_FONT_L,
        }
    }
};

const DashApp_ContentType pageLabels[1][3] = 
{
    {
        // Page 0
        {
            // Row 0
            .mode = DASHAPP_ADD,
            .posX = 1u,
            .posY = 11u,
            .string = "IAT:",
            .len = 4u,
            .ft = DASHAPP_FONT_S,
        },
        {
            // Row 1
            .mode = DASHAPP_ADD,
            .posX = 1u,
            .posY = 21u,
            .string = "Oil:",
            .len = 4u,
            .ft = DASHAPP_FONT_S,
        },
        {
            // Row 2
            .mode = DASHAPP_ADD,
            .posX = 1u,
            .posY = 31u,
            .string = "Fuel:",
            .len = 5u,
            .ft = DASHAPP_FONT_S
        }
    }
};

