
#include "stalkButtons.h"

static uint8_t lastButtons = 0;
static StalkButtons_Type events = STALKBUTTONS_NOEVENT;

StalkButtons_Type StalkButtons_Get(void)
{
    StalkButtons_Type retVal = STALKBUTTONS_NOEVENT;
    retVal = events;
    events = STALKBUTTONS_NOEVENT;
    return retVal;
}

void StalkButtons_Receive(uint8_t buttons)
{
    if (lastButtons != buttons)
    {
        if ((0 != (lastButtons & 0x20u)) && (0 == (buttons & 0x20u)))
        {
            // UP was pressed and now released
            events |= STALKBUTTONS_UP;
        }
        else if ((0 != (lastButtons & 0x10u)) && (0 == (buttons & 0x10u)))
        {
            // DOWN was pressed and now released
            events |= STALKBUTTONS_DOWN;
        }
        else if ((0 != (lastButtons & 0x40u)) && (0 == (buttons & 0x40u)))
        {
            // RESET/OK was pressed and now released
            events |= STALKBUTTONS_RESET;
        }
        else 
        {
            // Nothing interesting happened
        }
        lastButtons = buttons;
    }
}