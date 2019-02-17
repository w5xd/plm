/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */

#include "Keypad.h"

namespace w5xdInsteon {

Keypad::Keypad(PlmMonitor *plm, const unsigned char addr[3]) : Dimmer(plm, addr)
{}

Keypad::~Keypad()
{}

int Keypad::setX10Code(unsigned char houseCodeBin, unsigned char unitBin, unsigned char btn)
{
    /* This keypad implementation does not translate the house code and unit number per the X10
    ** scrambling. Instead, it sends the numbers unchanged to the device. 
    */
    if ((btn >= 1) && (btn <= 8))
        return sendExtendedCommand(btn, 0x4, houseCodeBin, unitBin);
    return -1;
}

}
