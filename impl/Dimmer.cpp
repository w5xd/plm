/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include "Dimmer.h"
#include "PlmMonitor.h"
#include <chrono>
namespace w5xdInsteon {

Dimmer::Dimmer(PlmMonitor *p, const unsigned char addr[3]) : 
    InsteonDevice(p, addr), 
        m_valueKnown(false), m_lastKnown(0),
        m_lastQueryCommandId(-1)
{}

int Dimmer::getDimmerValue(bool fromCache) 
{
    if (!fromCache)
    {
        unsigned char getStatus[8] =
        { 0x02, 0x62, 0, 0, 0, 0xF, 0x19, 0};
        memcpy(&getStatus[2], m_addr, 3);
        {
            auto start(std::chrono::steady_clock::now());
            std::unique_lock<std::mutex> l(m_mutex);
            std::shared_ptr<InsteonCommand> p = m_plm->queueCommand(getStatus, sizeof(getStatus), 9);
            m_lastQueryCommandId = p->m_globalId;
            m_valueKnown = false;
            static const auto secondsToWait = std::chrono::seconds(GET_VALUE_TIMEOUT_SECONDS);
            while (!m_valueKnown && (std::chrono::steady_clock::now() - start) < secondsToWait)
            {
                m_condition.wait_for(l, secondsToWait);
            }
        }
    }
    if (m_valueKnown) return m_lastKnown;
    return -10;
}

int Dimmer::setDimmerValue(unsigned char v)
{
    if ((v == 0) || (v == 255))
        if (m_plm->setIfLinked(this, v) >= 0)
            return 0;
    unsigned char lampSet[8]=
    { 0x02, 0x62, 0x11, 0x11, 0x11, 0x0F, 0x11, v};
    memcpy(&lampSet[2], m_addr, 3);
    m_plm->queueCommand(lampSet, sizeof(lampSet), 9);
    return 0;
}

int Dimmer::setDimmerFast(int v)
{
    if (m_plm->setFastIfLinked(this, v) >= 0)
            return 0;
    unsigned char lampSet[8]=
    { 0x02u, 0x62u, 0x11u, 0x11u, 0x11u, 0x0Fu, v > 0 ? 0x12u : 0x14u, 0};
    memcpy(&lampSet[2], m_addr, 3);
    m_plm->queueCommand(lampSet, sizeof(lampSet), 9);
    return 0;
}

void Dimmer::incomingMessage(const std::vector<unsigned char> &v, std::shared_ptr<InsteonCommand> p)
{
    InsteonDevice::incomingMessage(v, p); // let base class process message first
    if (v.size() >= 11)
    {
        if ((v[FLAG_BYTE] & 0xF0) == ACK_BIT) // is a P2P ACK
        {
            std::unique_lock<std::mutex> l(m_mutex);
            if ((v[COMMAND1] == 0x11) || 
                p->m_globalId == m_lastQueryCommandId)
            {
                m_valueKnown = true;;
                m_lastKnown = v[COMMAND2];
                m_condition.notify_all();
                if (m_plm->getErrorLevel() >= static_cast<int>(PlmMonitor::MESSAGE_ON))
                    m_plm->cerr()  << "Dimmer value "<< std::dec << (int)m_lastKnown << " received for device: " << 
                    m_addr <<                   std::endl;
                m_lastQueryCommandId = 0;
            }
        }
    }
}
}
