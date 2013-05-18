/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include "Dimmer.h"
#include "PlmMonitor.h"
#include <boost/date_time/posix_time/posix_time.hpp>
namespace w5xdInsteon {

Dimmer::Dimmer(PlmMonitor *p, const unsigned char addr[3]) : 
    InsteonDevice(p, addr, DEVICE_DIMMER), 
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
            boost::posix_time::ptime start(boost::posix_time::microsec_clock::universal_time());
            boost::mutex::scoped_lock l(m_mutex);
            boost::shared_ptr<InsteonCommand> p = m_plm->queueCommand(getStatus, sizeof(getStatus), 9);
            m_lastQueryCommandId = p->m_globalId;
            m_valueKnown = false;
            static const int secondsToWait = 5;
            while (!m_valueKnown && (boost::posix_time::microsec_clock::universal_time() - start).total_seconds() < secondsToWait)
            {
                m_condition.timed_wait(l, boost::posix_time::time_duration(0, 0, secondsToWait));
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
    { 0x02, 0x62, 0x11, 0x11, 0x11, 0x0F, v > 0 ? 0x12 : 0x14, 0};
    memcpy(&lampSet[2], m_addr, 3);
    m_plm->queueCommand(lampSet, sizeof(lampSet), 9);
    return 0;
}

void Dimmer::incomingMessage(const std::vector<unsigned char> &v, boost::shared_ptr<InsteonCommand> p)
{
    InsteonDevice::incomingMessage(v, p); // let base class process message first
    if (v.size() >= 11)
    {
        if ((v[FLAG_BYTE] & 0xF0) == ACK_BIT) // is a P2P ACK
        {
            boost::mutex::scoped_lock l(m_mutex);
            if ((v[COMMAND1] == 0x11) || 
                p->m_globalId == m_lastQueryCommandId)
            {
                m_valueKnown = true;;
                m_lastKnown = v[COMMAND2];
                m_condition.notify_all();
                if (m_plm->getErrorLevel() >= static_cast<int>(PlmMonitor::MESSAGE_ON))
                    m_plm->cerr()  << "Dimmer value "<< std::dec << (int)m_lastKnown << " received for device: " << 
                    m_addr <<                   std::endl;
            }
        }
    }
}

}
