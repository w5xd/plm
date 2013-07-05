/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITOR_FANLINC_H
#define MOD_W5XDINSTEON_PLMMONITOR_FANLINC_H

#include "Dimmer.h"

namespace w5xdInsteon {
class Fanlinc :   public Dimmer
{
public:
    Fanlinc(PlmMonitor *plm, const unsigned char addr[3]);
    ~Fanlinc();

    int setFanSpeed(unsigned char v);   /* range of 0 to 0xff */

    int getFanSpeed() ;

};
}
#endif
