/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */

#include "Keypad.h"

namespace w5xdInsteon {

Keypad::Keypad(PlmMonitor *plm, const unsigned char addr[3]) : Dimmer(plm, addr)
{}

Keypad::~Keypad()
{}

}
