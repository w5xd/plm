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
    InitExtMsg(extMsg, 0x11);
    memset(&extMsg[OFFSET_D1], 0, 14);
    memcpy(&extMsg[OFFSET_TO_ADDR], m_addr, sizeof(m_addr));
    extMsg[OFFSET_CMD2] = v;  
    extMsg[OFFSET_D1] = 2;
    m_plm->queueCommand(extMsg, sizeof(extMsg), 23); 
    return 0;
}

int Fanlinc::getFanSpeed()
{
        unsigned char getStatus[8] =
        { 0x02, 0x62, 0, 0, 0, 0xF, 0x19, 3};
        memcpy(&getStatus[2], m_addr, 3);
        {
            auto start(std::chrono::steady_clock::now());
            std::unique_lock<std::mutex> l(m_mutex);
            std::shared_ptr<InsteonCommand> p = m_plm->queueCommand(getStatus, sizeof(getStatus), 9);
            m_lastQueryCommandId = p->m_globalId;
            m_valueKnown = false;
            static const auto secondsToWait = std::chrono::seconds(5);
            while (!m_valueKnown && (std::chrono::steady_clock::now() - start) < secondsToWait)
            {
                m_condition.wait_for(l, secondsToWait);
            }
            if (m_valueKnown) return m_lastKnown;
        }
        return -10;
}

}
