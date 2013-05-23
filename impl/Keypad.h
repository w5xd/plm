/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITOR_KEYPAD_H
#define MOD_W5XDINSTEON_PLMMONITOR_KEYPAD_H

#include "Dimmer.h"

namespace w5xdInsteon {
class Keypad :   public Dimmer
{
public:
    Keypad(PlmMonitor *plm, const unsigned char addr[3]);
    ~Keypad();

    int setKeypadFollowMask(unsigned char button, unsigned char mask)
    {
        return sendExtendedCommand(button, 2, mask);
    }

    int setKeypadOffMask(unsigned char button, unsigned char mask)
    {
        return sendExtendedCommand(button, 3, mask);
    }

    int setNonToggleState(unsigned char button, unsigned char nonToggle)
    {
        return sendExtendedCommand(button, 8, nonToggle);
    }

    int setX10Code(char houseCode, unsigned char unit, unsigned char btn)
    {
        if ((btn >= 1) && (btn <= 8))
            return InsteonDevice::setX10Code(houseCode, unit, btn);
        return -1;
    }

   // doesn't appear to work with 2466 togglelinc dimmer. Does work with keypadlinc
    int setWallLEDbrightness(unsigned char bright)   {return sendExtendedCommand(1, 0x7, bright);}

};
}
#endif
