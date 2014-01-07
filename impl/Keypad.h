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

    int setKeypadFollowMask(unsigned char btn, unsigned char mask)
    {
        if ((btn >= 1) && (btn <= 8))
            return sendExtendedCommand(btn, 2, mask);
        return -1;
    }

    int setKeypadOffMask(unsigned char btn, unsigned char mask)
    {
        if ((btn >= 1) && (btn <= 8))
            return sendExtendedCommand(btn, 3, mask);
        return -1;
    }

    int setNonToggleState(unsigned char btn, unsigned char nonToggle)
    {
        if ((btn >= 1) && (btn <= 8))
            return sendExtendedCommand(btn, 8, nonToggle);
        return -1;
    }

    int setRampRate(unsigned char btn, unsigned char rate)
    {
        if ((btn >= 1) && (btn <= 8))
            return sendExtendedCommand(btn, 5, rate);
        return -1;
    }

    int setOnLevel(unsigned char btn, unsigned char level)
    {
        if ((btn >= 1) && (btn <= 8))
            return sendExtendedCommand(btn, 6, level);
        return -1;
    }

    int getX10Code(char &houseCode, unsigned char &unit, unsigned char btn) const 
    { 
        if ((btn >= 1) && (btn <= 8))
            return InsteonDevice::getX10Code(houseCode, unit, btn);
        return -1;
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
