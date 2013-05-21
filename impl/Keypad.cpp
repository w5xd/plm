#include "Keypad.h"

namespace w5xdInsteon {

Keypad::Keypad(PlmMonitor *plm, const unsigned char addr[3]) : Dimmer(plm, addr)
{
}

Keypad::~Keypad()
{
}
}
