#include "dis_cfg.h"
#include "dashapp.h"

const DiagIdType pageDiag[1][3] = 
{
    // Page0
    {
        // Row 0
        {
            #if (1 == CONFIG_BENCH_TEST_MODE)
            .diagCh = DASHDIAG_CH_VEHICLESPEED,
            #else
            .diagCh = ENGINEDIAG_CH_IATTEMP7, // did 7
            #endif
            .timeout = 2000
        },
        // Row 1
        {
            #if (1 == CONFIG_BENCH_TEST_MODE)
            .diagCh = DASHDIAG_CH_OILPRESSURE,
            #else
            .diagCh = ENGINEDIAG_CH_OILTEMP, // did 29
            #endif
            .timeout = 10000
        },
        // Row 2
        {
            #if (1 == CONFIG_BENCH_TEST_MODE)
            .diagCh = DASHDIAG_CH_FUELLEVEL1,
            #else
            .diagCh = ENGINEDIAG_CH_FUELTEMP, // did7
            #endif
            .timeout = 10000
        }
        
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
            #if (1 == CONFIG_BENCH_TEST_MODE)
            .string = "Speed:",
            #else
            .string = "IAT:",
            #endif
            .len = 5u,
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

