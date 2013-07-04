/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */

#include "Fanlinc.h"
#include "PlmMonitor.h"

namespace w5xdInsteon {

Fanlinc::Fanlinc(PlmMonitor *plm, const unsigned char addr[3]) : Dimmer(plm, addr)
{}

Fanlinc::~Fanlinc()
{}

int Fanlinc::setFanSpeed(unsigned char v)
{
    unsigned char extMsg[EXTMSG_COMMAND_LEN];
    InitExtMsg(extMsg);
    memset(&extMsg[OFFSET_D1], 0, 14);
    memcpy(&extMsg[OFFSET_TO_ADDR], m_addr, sizeof(m_addr));
    extMsg[OFFSET_CMD1] = 0x11; 
    extMsg[OFFSET_CMD2] = v;  
    extMsg[OFFSET_D1] = 2;
    PlaceCheckSum(extMsg);
    m_plm->queueCommand(extMsg, sizeof(extMsg), 23); 
    return 0;
}

}
