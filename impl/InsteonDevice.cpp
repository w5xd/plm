/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include <cstring>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "InsteonDevice.h"
#include "PlmMonitor.h"

namespace w5xdInsteon {

const unsigned char InUseFlag = 0x80;

enum {OFFSET_TO_ADDR = 2, 
        OFFSET_FLAG = 5,
        OFFSET_CMD1 = 6,
        OFFSET_CMD2 = 7,
        OFFSET_D1 = 8,
        OFFSET_D2 = 9,
        OFFSET_D3 = 10,
        OFFSET_D4 = 11,
        OFFSET_D5 = 12,
        OFFSET_LINK_FLAG = 13,
        OFFSET_D6 = 13,
        OFFSET_LINK_GROUP = 14,
        OFFSET_LINK_ADDR = 15,
        OFFSET_LINK_LS1 = 18,
        OFFSET_LINK_LS2 = 19,
        OFFSET_LINK_LS3 = 20,
        EXTMSG_COMMAND_LEN = 22};

enum {EXT_MSG_LENGTH = 23,
    EXT_D1=9,
    EXT_D2=10,
    EXT_D3=11,
    EXT_D4=12,
    EXT_D5=13,
    EXT_D6=14,
    };

void PlaceCheckSum(unsigned char *extMsg)
{
    // This sum is undocumented, but required for, apparently,
    // "all extended commands"
    unsigned char x = 0;
    for (int i = 6; i <= 20; i++) x += extMsg[i];
    extMsg[21] = ~x + 1;
}

bool InsteonDeviceAddr::operator <(const InsteonDeviceAddr &other) const
{
    if (m_addr[0] < other.m_addr[0])
        return true;
    if (m_addr[0] == other.m_addr[0])
    {
        if (m_addr[1] < other.m_addr[1])
            return true;
        if (m_addr[1] == other.m_addr[1])
        {
            if (m_addr[2] < other.m_addr[2])
                return true;
        }
    }
    return false;
}

const unsigned char InsteonLinkEntry::CONTROLLER_FLAG = 0x40;

void InsteonLinkEntry::print(std::ostream &oss)const
{
    oss << std::hex << (int)m_flag << " ";
    if (m_flag & InsteonLinkEntry::CONTROLLER_FLAG)  oss << "c"; else oss << "r";
    oss << " " <<
        (int)m_group << " ";
    bufferToStream(oss, m_addr, 3);
    oss << " " << (int)m_LinkSpecific1 << 
        " " << (int)m_LinkSpecific2 << 
        " " << (int)m_LinkSpecific3;
}

InsteonDevice::InsteonDevice(PlmMonitor *p, const unsigned char addr[3]) :
    m_plm(p)
   ,m_LinkTableComplete(false)
   ,m_requestedOneLink(false)
   ,m_finalAddr(0)
   ,m_lastAcqCommand1(0)
   ,m_incomingMessageCount(0)
{
    m_addr = addr;
}

bool InsteonDevice::operator < (const InsteonDevice &other) const
{    return m_addr < other.m_addr;}

void InsteonDevice::incomingMessage(const std::vector<unsigned char> &v, boost::shared_ptr<InsteonCommand>)
{
    {
        boost::mutex::scoped_lock l(m_mutex);
        m_incomingMessageCount += 1;
    }
    static const unsigned char ack[3] = {0x02, 0x68, 0};
    if (v.size() >= 25)
    {
        static const unsigned char StartExtended[] = { 0x2, 0x51};
        if (memcmp(&v[START_BYTE], StartExtended, sizeof(StartExtended)) == 0)
        {
            if ((v[FLAG_BYTE] & 0xF0) == EXTMSG_BIT)
            {
                if ((v[COMMAND1] == 0x2f) && (v[COMMAND2] == 0))
                {
                    unsigned which = v[COMMAND2+3];
                    which <<= 8;
                    which |= v[COMMAND2+4];
                    unsigned char flag = v[COMMAND2+6];
                    boost::mutex::scoped_lock l(m_mutex);
                    if (!m_LinkTableComplete)
                    {
#ifdef _DEBUGXXX   // force retry of a particular record to test the retry code
                            if ((which == 0xfe7) && !m_requestedOneLink)
                                ;
                            else
#endif
                            if (flag & InUseFlag) 
                                m_LinkTable[which] = InsteonLinkEntry(&v[COMMAND2+6]);
                            else if (flag != 0)
                                m_UnusedLinks.insert(which);
                            else
                                m_finalAddr = which;
                    }
                    // see if we got them all
                    if ((flag == 0) || m_requestedOneLink)
                    {
                        for (unsigned maddr = 0xfff; maddr != m_finalAddr; maddr -= 8)
                        {
                            if (m_LinkTable.find(maddr) == m_LinkTable.end() &&
                                m_UnusedLinks.find(maddr) == m_UnusedLinks.end())
                            {
                                // oops.. didn't get them all
                                m_requestedOneLink = true;
                                reqAllLinkData(maddr);
                                return;
                            }
                        }
                        m_LinkTableComplete = true;
                        m_condition.notify_all();
                    }
                }
                else if ((v[COMMAND1] == 0x2e) && (v[COMMAND2] == 0))
                {
                    boost::mutex::scoped_lock l(m_mutex);
                    ExtendedGetResults_t::iterator itor = m_ExtendedGetResult.insert(
                        ExtendedGetResults_t::value_type(v[COMMAND2 + 1], 
                        ExtendedGetResults_t::mapped_type())).first;
                    itor->second.assign(v.begin()+2, v.end()); 
                    m_condition.notify_all();
                }
                else if ((v[COMMAND1] == 0x3) && (v[COMMAND2] == 0))
                {
                    m_productData.assign(v.begin()+2, v.end());
                    m_condition.notify_all();
                }
            }
            else if ((v[FLAG_BYTE] & 0xF0) == (EXTMSG_BIT|ACK_BIT))
            {
            }
        }
    } else if (v.size() >= 11)
    {   
        static const unsigned char StartStandard[] = { 0x2, 0x50};
        if ((memcmp(&v[START_BYTE], StartStandard, sizeof(StartStandard)) == 0))
        {
            if ((v[FLAG_BYTE] & 0xF0) == GROUP_BIT)
            { // acq this group clean-up message
               m_plm->queueCommand(ack, sizeof(ack), 4, false); 
            }
            else if ((v[FLAG_BYTE] & 0xF0) == ACK_BIT)
            {
                boost::mutex::scoped_lock l(m_mutex);
                m_lastAcqCommand1 = v[COMMAND1];
                m_condition.notify_all();
            }
        }
    }
}

int InsteonDevice::numberOfLinks(int msecToWait) const
{
    boost::posix_time::ptime start(boost::posix_time::microsec_clock::universal_time());
    boost::mutex::scoped_lock l(m_mutex);
    while (!m_LinkTableComplete && (boost::posix_time::microsec_clock::universal_time() - start).total_milliseconds() < msecToWait)
    {
        int countBefore = m_incomingMessageCount;
        m_condition.timed_wait(l, boost::posix_time::time_duration(0, 0, 1 + msecToWait/1000));
        if (!m_LinkTableComplete)
        {
            if (countBefore != m_incomingMessageCount)   // If something changed, then reset the clock
                start = boost::posix_time::microsec_clock::universal_time();
        }
    }
    return m_LinkTableComplete ? m_LinkTable.size() : -1;
}

const char * InsteonDevice::printLinkTable() 
{
    boost::mutex::scoped_lock l(m_mutex);
    if (!m_LinkTableComplete) 
    {
       m_plm->cerr() << "No link table retrieved yet" << std::endl;
       return 0;
    }
    std::ostringstream linkTablePrinted;
    linkTablePrinted << "printLinkTable for device " << m_addr << std::endl << "addr flag group ID ls1 ls2 ls3" << std::endl;
    for (LinkTable_t::const_iterator itor = m_LinkTable.begin();
            itor != m_LinkTable.end();
            itor++)
    {
        linkTablePrinted << std::hex << (int)itor->first << " " ;
        itor->second.print(linkTablePrinted);
        linkTablePrinted << std::endl;
    }
    linkTablePrinted << "end table" << std::endl << std::flush;
    m_linkTablePrinted = linkTablePrinted.str();
    return m_linkTablePrinted.c_str();
}

void InsteonDevice::reqAllLinkData(unsigned addr)
{
    unsigned char reqAllLinkDatab[EXTMSG_COMMAND_LEN] =
    { 0x02, 0x62, 0x11, 0x11, 0x11, 0x1F, 0x2F, 0x00, };
    memcpy(&reqAllLinkDatab[2], m_addr, 3);
    if (addr != 0)
    {
        reqAllLinkDatab[OFFSET_D5] = 8;  // request one record
        reqAllLinkDatab[OFFSET_D3] = (addr >> 8) & 0xFF;
        reqAllLinkDatab[OFFSET_D4] = addr & 0xFF;
    }
    PlaceCheckSum(reqAllLinkDatab);
    m_plm->queueCommand(reqAllLinkDatab, sizeof(reqAllLinkDatab), 23);
}

int InsteonDevice::startGatherLinkTable()
{
    {           
        boost::mutex::scoped_lock l(m_mutex);
        m_LinkTable.clear();
        m_UnusedLinks.clear();
        m_LinkTableComplete = false;
        m_finalAddr = 0;
    }
    reqAllLinkData(0);
    return 1;
}

int InsteonDevice::linkPlm(bool amController, unsigned char grp, unsigned char ls1, unsigned char ls2, unsigned char ls3)
{
     int res = m_plm->createLink(this, !amController, grp, 0x80, ls1, ls2, ls3);
     if (res < 0)
            return -1;    

    return createLinkWithModem(grp, amController, ls1, ls2, ls3);
}

int InsteonDevice::unLinkPlm(bool amController, unsigned char grp, unsigned char ls3)
{
     int res = m_plm->removeLink(this, !amController, grp);
     int res2 =  removeLinks(m_plm->insteonID(), grp,  amController, ls3);
     return std::min(res, res2);
}


int InsteonDevice::createLinkWithModem(unsigned char group, bool amController, InsteonDevice *other, unsigned char og)
{
    for (LinkTable_t::const_iterator itor = m_LinkTable.begin(); itor != m_LinkTable.end(); itor++)
    {
        if ((itor->second.m_addr == other->m_addr) && (itor->second.m_group == og))
        {
            if (!((itor->second.m_flag & InsteonLinkEntry::CONTROLLER_FLAG ? true : false) ^ amController))
            {
                return createLinkWithModem(group, amController, itor->second.m_LinkSpecific1,
                                                                itor->second.m_LinkSpecific2,
                                                                itor->second.m_LinkSpecific3);
            }
        }
    }
    return -1;
}


static void InitExtMsg(unsigned char *extMsg)
{
    unsigned char init[EXTMSG_COMMAND_LEN] =
    { 0x02, 0x62, 
        0x11, 0x11, 0x11,   // to addr
        0x1F, 0x2F, 0x00,   // flag, cmd1, cmd2
        0x01, 0x02,     // d1, d2
        0x0f, 0xf7,     // d3, d4
        0x08,           // d5
        0xa0, // flag,
        0,    // group#  
            0, 0, 0, // addr linked
            0, 0, 0, // ls1, ls2, ls3
             0x00};
    memcpy(extMsg, init, sizeof(init));
}

int InsteonDevice::extendedGet(unsigned char btn, unsigned char *pBuf, unsigned bufsize)
{
    unsigned char extMsg[EXTMSG_COMMAND_LEN];
    InitExtMsg(extMsg);
    memcpy(&extMsg[OFFSET_TO_ADDR], m_addr, sizeof(m_addr));
    extMsg[OFFSET_CMD1] = 0x2e; // extended Set/Get
    extMsg[OFFSET_CMD2] = 0; 
    memset(&extMsg[OFFSET_D1], 0, 14);
    extMsg[OFFSET_D1] = btn;
    extMsg[OFFSET_D2] = 0;  // data request
    PlaceCheckSum(extMsg);
    ExtendedGetResults_t::iterator itor;
    {
        boost::mutex::scoped_lock l(m_mutex);
        itor = m_ExtendedGetResult.find(btn);
        if (itor != m_ExtendedGetResult.end())
            m_ExtendedGetResult.erase(itor);
    }
    static const int secondsToWait = 5;
    boost::mutex::scoped_lock l(m_mutex);
    itor = m_ExtendedGetResult.end();
    int attempts = 2;
    while (itor == m_ExtendedGetResult.end() && (attempts-- >= 0))
    {
        boost::posix_time::ptime start(boost::posix_time::microsec_clock::universal_time());
        boost::shared_ptr<InsteonCommand> p = m_plm->queueCommand(extMsg, sizeof(extMsg), 23); 
        while (itor == m_ExtendedGetResult.end() && 
            (boost::posix_time::microsec_clock::universal_time() - start).total_seconds() < secondsToWait)
        {
            int msgCount = m_incomingMessageCount;
            m_condition.timed_wait(l, boost::posix_time::time_duration(0, 0, secondsToWait));
            itor = m_ExtendedGetResult.find(btn); 
            if ((msgCount != m_incomingMessageCount) || (p->m_timesSent == 0))
                start = boost::posix_time::microsec_clock::universal_time();
            
            if (static_cast<int>(m_plm->m_verbosity) >= PlmMonitor::MESSAGE_EVERY_IO)
                m_plm->cerr() << std::dec<< "Get: at " << start << " btn " << (int)btn << 
                    " msgCount " << msgCount << " attempts " << attempts <<
                    " m_incomingMessageCount " << m_incomingMessageCount << 
                    (itor == m_ExtendedGetResult.end() ? " not found" : " found") << std::endl;
        }
    }
    if (itor == m_ExtendedGetResult.end())
        return -1;
    unsigned ret = itor->second.size();
    static const int IGNORE_CHARS = 9;
    if (ret >= IGNORE_CHARS)
        ret -= IGNORE_CHARS;
    else
        return -2;
    if (!pBuf)
        return 0;   // successfully return nothing
    if (ret > bufsize)
        ret = bufsize;
    if (ret)
        memcpy(pBuf, &itor->second[IGNORE_CHARS], ret);
    return  ret;
}

const char * InsteonDevice::printExtendedGet(unsigned char btn)
{
    boost::mutex::scoped_lock l(m_mutex);
    ExtendedGetResults_t::const_iterator itor = m_ExtendedGetResult.find(btn);
    if (itor == m_ExtendedGetResult.end() ) 
    {
       m_plm->cerr() << "No extended get/set response retrieved yet" << std::endl;
       return 0;
    }
    std::ostringstream oss;
    oss << "Extended state for device " << m_addr << std::endl <<
"                                             D1   D2   D3   D4   D5   D6   D7   D8" << std::endl <<
"                                             btn  resp unu  unu  X10H X10U RAMP ONLV" << std::endl;
    bufferToStream(oss, &itor->second[0], itor->second.size());
    oss << std::endl;
    m_extendedGetPrint[btn] = oss.str();
    return m_extendedGetPrint[btn].c_str();
}

int InsteonDevice::createLinkWithModem(unsigned char group, bool amController, 
                                       unsigned char ls1, unsigned char ls2, unsigned char ls3)
{
    if (!linktableComplete()) return -2;
    // Calculate d3 and d4 as the next link byte offset to use
    // Notice if the controllers already have a link to the same addr
    // and group and overwrite that one.
    InsteonDeviceAddr plmAddr(m_plm->insteonID());
    int addr = linkAddr(plmAddr, group, amController, ls3);
    unsigned char extMsg[EXTMSG_COMMAND_LEN];
    InitExtMsg(extMsg);
    extMsg[OFFSET_LINK_GROUP] = group;
    extMsg[OFFSET_LINK_LS1] = ls1;
    extMsg[OFFSET_LINK_LS2] = ls2;
    extMsg[OFFSET_LINK_LS3] = ls3;

    extMsg[OFFSET_D3] = static_cast<unsigned char>(addr >> 8);
    extMsg[OFFSET_D4] = static_cast<unsigned char>(addr);

    memcpy(&extMsg[OFFSET_TO_ADDR], m_addr, sizeof(m_addr));
    memcpy(&extMsg[OFFSET_LINK_ADDR], plmAddr, 3);
    extMsg[OFFSET_LINK_FLAG] = amController ? 0xe2 : 0xa2;  // flag as Controller/responder
    PlaceCheckSum(extMsg);
    if (m_plm->m_verbosity >= static_cast<int>(m_plm->MESSAGE_ON))
    {
        m_plm->cerr() << "Attemping link on address " << std::hex << addr << " and group " << (int)group << std::endl;
    }

    boost::shared_ptr<InsteonCommand> p = m_plm->sendCommandAndWait(extMsg, sizeof(extMsg), 23); 
    return p->m_answerState;
}

int InsteonDevice::createLink(InsteonDevice *responder, unsigned char group,
                              unsigned char ls1, unsigned char ls2, unsigned char ls3)
{
    // creates 2 links. One in "this" as controller, and the other
    // in "responder" as responder. ls1, ls2, ls3 are added to the link table on both
    // controller and responder. (Because their values on the controller appear to be irrelevant)
    if (memcmp(&m_addr[0], &responder->m_addr[0], sizeof(m_addr)) == 0)
        return -1; // can't link to self
    if (!linktableComplete()) return -2;
    if (!responder->linktableComplete()) return -3;

    unsigned char extMsg[EXTMSG_COMMAND_LEN];
    InitExtMsg(extMsg);
    extMsg[OFFSET_LINK_GROUP] = group;
    extMsg[OFFSET_LINK_LS1] = ls1;
    extMsg[OFFSET_LINK_LS2] = ls2;
    extMsg[OFFSET_LINK_LS3] = ls3;

    // Calculate d3 and d4 as the next link byte offset to use
    // Notice if the controllers already have a link to the same addr
    // and group and overwrite that one.
    int addr = linkAddr(responder->m_addr, group, true, ls3);
    extMsg[OFFSET_D3] = static_cast<unsigned char>(addr >> 8);
    extMsg[OFFSET_D4] = static_cast<unsigned char>(addr);

    memcpy(&extMsg[OFFSET_TO_ADDR], m_addr, sizeof(m_addr));
    memcpy(&extMsg[OFFSET_LINK_ADDR], responder->m_addr, sizeof(responder->m_addr));
    extMsg[OFFSET_LINK_FLAG] = 0xea;  // flag as Controller
    PlaceCheckSum(extMsg);
    boost::shared_ptr<InsteonCommand> p = m_plm->sendCommandAndWait(extMsg, sizeof(extMsg), 23); // controller

    addr = responder->linkAddr(m_addr, group, false, ls3);
    extMsg[OFFSET_D3] = static_cast<unsigned char>(addr >> 8);
    extMsg[OFFSET_D4] = static_cast<unsigned char>(addr);
    memcpy(&extMsg[OFFSET_LINK_ADDR], m_addr, sizeof(m_addr));
    memcpy(&extMsg[OFFSET_TO_ADDR], responder->m_addr, sizeof(responder->m_addr));
    extMsg[OFFSET_LINK_FLAG] = 0xaa;  // flag as Responder
    PlaceCheckSum(extMsg);
    p = m_plm->sendCommandAndWait(extMsg, sizeof(extMsg), 23);

    return 1;
}

int InsteonDevice::createModemGroupToMatch(int grp)
{
    int ret = 0;
    static const int GROUP_1 = 1;
    if (!m_LinkTableComplete) return -1;
    std::vector<InsteonDevice *> othersToLink;
    {
        boost::mutex::scoped_lock l(m_mutex);
        for (LinkTable_t::iterator itor = m_LinkTable.begin(); itor != m_LinkTable.end(); itor++)
        {
            InsteonLinkEntry &link = itor->second;
            if ((link.m_flag & InsteonLinkEntry::CONTROLLER_FLAG) && (link.m_group == GROUP_1))
            {   // controller on group 1
                othersToLink.push_back(m_plm->getDeviceAccess<InsteonDevice>(link.m_addr));
            }
        }
    }
    if (othersToLink.empty()) return 0;
    for (unsigned i = 0; i < othersToLink.size(); i++)
    {
        othersToLink[i]->startGatherLinkTable();
        if (othersToLink[i]->numberOfLinks() <= 0)
            return -3;
    }

    if (grp <= 0)
        grp = m_plm->nextUnusedControlGroup();
    if (grp <= 0)
        return -2;

    // try to create a link in our modem first
    // The ls3 == 1 means "this link can be commanded instead of the individual device 
    int res = m_plm->createLink(this, true, grp, 0xA0, 0, 0, 1);
    if (res < 0)
        return -4;
    for (unsigned i = 0; i < othersToLink.size(); i++)
        if (m_plm->createLink(othersToLink[i], true, grp, 0xA0, 0, 0, 0) < 0)
            return -5;

    createLinkWithModem(grp, false, 0xff, 0, 0);
    for (unsigned i = 0; i < othersToLink.size(); i++)
        othersToLink[i]->createLinkWithModem(grp, false, this, GROUP_1);
    return ret;
}

int InsteonDevice::linkAddr(const InsteonDeviceAddr &dev, unsigned char grp, bool isControl, unsigned char ls3) const
{
    int addr = 0xfff;   // first address per documentation
    if (!m_LinkTable.empty())
        addr = m_LinkTable.begin()->first - 8;  // one previous to existing smallest number
    int nextAddr = 0xfff;
    for (LinkTable_t::const_reverse_iterator itor = m_LinkTable.rbegin(); itor != m_LinkTable.rend(); itor++)
    {
        if (itor->first != nextAddr)    // there is a gap here. use it
            addr = nextAddr;
        if ((itor->second.m_addr == dev) && (itor->second.m_group == grp))
        {
            if (!((itor->second.m_flag & InsteonLinkEntry::CONTROLLER_FLAG ? true : false) ^ isControl)) // control bits match?
            {
                if (isControl || (ls3 == 0) || (ls3 == itor->second.m_LinkSpecific3))
                {   // for responders, the ls3 info must match in order to override.
                    // ls3 is "part of" the responder that is to respond.
                    // Caller setting ls3 to zero matches all "parts"
                    addr = itor->first;
                    break;
                }
            }
        }
        nextAddr = itor->first - 8; 
    }
    return addr;
}

int InsteonDevice::removeLink(InsteonDevice *responder, unsigned char group, unsigned char ls3)
{
    int r1 = responder->removeLinks(m_addr, group, false, ls3);
    int r2 = 0;
    if (!responder->amRespondingTo(m_addr, group))   // only if removed last responder link does controller  also get removed
        r2 = removeLinks(responder->m_addr, group, true, ls3);
    if (r1 <= 0)
        return r1;
    if (r2 <= 0)
        return r2;
    return 0;
}

bool InsteonDevice::amRespondingTo(const InsteonDeviceAddr &addr, unsigned char group)const
{
    for (LinkTable_t::const_reverse_iterator itor = m_LinkTable.rbegin(); itor != m_LinkTable.rend(); itor++)
    {
        const InsteonLinkEntry &link = itor->second;
        if ((link.m_flag & InUseFlag) &&
            (link.m_addr == addr) && 
            (link.m_group == group) &&
            !(link.m_flag & InsteonLinkEntry::CONTROLLER_FLAG ? true : false)
            )
            return true;
    }
    return false;
}

int InsteonDevice::removeLinks(const InsteonDeviceAddr &addr, unsigned char group, bool amController, unsigned char ls3)
{
    if (m_LinkTable.empty()) 
        return 0;
    for (LinkTable_t::reverse_iterator itor = m_LinkTable.rbegin(); itor != m_LinkTable.rend(); itor++)
    {
        InsteonLinkEntry &link = itor->second;
        if ((link.m_flag & InUseFlag) &&
            (link.m_addr == addr) && 
            (link.m_group == group) &&
            !((link.m_flag & InsteonLinkEntry::CONTROLLER_FLAG ? true : false) ^ amController) &&
            (amController || (ls3 == 0) || (ls3 == link.m_LinkSpecific3)) // responder only matches if ls3 matches as well
            )
        {
                unsigned char extMsg[EXTMSG_COMMAND_LEN];
                unsigned int maddr = itor->first;
                InitExtMsg(extMsg);
                extMsg[OFFSET_LINK_GROUP] = link.m_group;
                extMsg[OFFSET_LINK_LS1] = link.m_LinkSpecific1;
                extMsg[OFFSET_LINK_LS2] = link.m_LinkSpecific2;
                extMsg[OFFSET_LINK_LS3] = link.m_LinkSpecific3;
                extMsg[OFFSET_D3] = static_cast<unsigned char>(maddr >> 8);
                extMsg[OFFSET_D4] = static_cast<unsigned char>(maddr);

                memcpy(&extMsg[OFFSET_TO_ADDR], m_addr, sizeof(m_addr));
                memcpy(&extMsg[OFFSET_LINK_ADDR], link.m_addr, 3);
                extMsg[OFFSET_LINK_FLAG] = link.m_flag & ~InUseFlag;  // clear the InUseFlag
                PlaceCheckSum(extMsg);
                m_lastAcqCommand1 = 0;
                boost::shared_ptr<InsteonCommand> p = m_plm->sendCommandAndWait(extMsg, sizeof(extMsg), 23); 
                if (p->m_answerState < 0)
                    return p->m_answerState;
                else
                {
                    link.m_flag &= ~InUseFlag;

                    // hang around for 5 seconds or until receive an acq, which is first
                    static const int secondsToWait = 5;
                    boost::posix_time::ptime start(boost::posix_time::microsec_clock::universal_time());
                    boost::mutex::scoped_lock l(m_mutex);
                    while ((m_lastAcqCommand1 != 0x2f) && (boost::posix_time::microsec_clock::universal_time() - start).total_seconds() < secondsToWait)
                    {
                        m_condition.timed_wait(l, boost::posix_time::time_duration(0, 0, secondsToWait));
                    }
                }
        }
    }
    return 0;
}

// http://software.x10.com/pub/manuals/xtdcode.pdf
const char InsteonDevice::X10HouseCodeToLetter[] =
{    'M', 'E', 'C', 'K', 'O', 'G', 'A', 'I', 'N', 'F', 'D', 'L', 'P', 'H', 'B', 'J'};
const char InsteonDevice::X10HouseLetterToBits[] = 
{    6, 0xe, 2, 0xa, 1, 9, 5, 0xd, 7, 0xf, 3, 0xb, 0, 8, 4, 0xc};
const char InsteonDevice::X10BitsToWheelCode[]=
{   13, 5, 3, 11, 15, 7, 1, 9, 14, 6, 4, 12, 16, 8, 2, 10};
const char InsteonDevice::X10WheelCodeToBits[] =
{   0, 0x0c, 0x1c, 0x4, 0x14, 0x2, 0x12, 0xa, 0x1a, 0xe, 0x1e, 0x6, 0x16, 0x0, 0x10, 0x8, 0x18};

int InsteonDevice::getX10Code(char &houseCode, unsigned char &unit, unsigned char btn) const
{
    ExtendedGetResults_t::const_iterator itor = m_ExtendedGetResult.find(btn);
    if (itor == m_ExtendedGetResult.end())
        return -1;
    if (itor->second.size() < EXT_MSG_LENGTH)
        return -2;
    unsigned char hc = itor->second[EXT_D5];
    unsigned char u = itor->second[EXT_D6];
    if ((hc == 0x20) || (u == 0x20))
    {
        houseCode = 0;
        unit = 0;
    }
    else
    {
        houseCode = X10HouseCodeToLetter[hc & 0xF];
        unit = X10BitsToWheelCode[0xf & (u >> 1)];
    }
    return 1;
}

int InsteonDevice::setX10Code(char houseCode, unsigned char unit, unsigned char btn)
{
    unsigned char hc = 0x20, u = 0x20;
    if ((houseCode != 0) || (unit != 0))
    {
        if ((houseCode < 'A') || (houseCode > 'P'))
            return -12;
        if ((unit < 1) || (unit > 16))
            return -13;
        hc = X10HouseLetterToBits[houseCode - 'A'];
        u = X10WheelCodeToBits[unit];
    }
    return sendExtendedCommand(btn, 0x4, hc, u);
}

int InsteonDevice::sendExtendedCommand(unsigned char btn, unsigned char d2, unsigned char d3, unsigned char d4)
{
    unsigned char extMsg[EXTMSG_COMMAND_LEN];
    InitExtMsg(extMsg);
    memset(&extMsg[OFFSET_D1], 0, 14);
    memcpy(&extMsg[OFFSET_TO_ADDR], m_addr, sizeof(m_addr));
    extMsg[OFFSET_CMD1] = 0x2e; // extended Set/Get
    extMsg[OFFSET_CMD2] = 0;  
    extMsg[OFFSET_D1] = btn;
    extMsg[OFFSET_D2] = d2;      // SET command--X10
    extMsg[OFFSET_D3] =  d3; 
    extMsg[OFFSET_D4] =  d4;
    extMsg[OFFSET_D5] = 0;
    extMsg[OFFSET_D6] = 0;
    PlaceCheckSum(extMsg);
    m_plm->queueCommand(extMsg, sizeof(extMsg), 23); 
    return 0;
}

int InsteonDevice::getProductData() 
{   // keypad responds. togglelinc does not
    unsigned char getPd[8] =
    { 0x02, 0x62, 0, 0, 0, 0xF, 0x3, 0};
    memcpy(&getPd[2], m_addr, 3);
    boost::mutex::scoped_lock l(m_mutex);
    boost::posix_time::ptime start(boost::posix_time::microsec_clock::universal_time());
    m_productData.clear();
    boost::shared_ptr<InsteonCommand> p = m_plm->queueCommand(getPd, sizeof(getPd), 9);
    static const int secondsToWait = 5;
    while (m_productData.empty() && 
        (boost::posix_time::microsec_clock::universal_time() - start).total_seconds() < secondsToWait)
    {
        m_condition.timed_wait(l, boost::posix_time::time_duration(0, 0, secondsToWait));
    }
    return m_productData.size();
}

std::ostream & operator << (std::ostream &os, const InsteonDeviceAddr &dev)
{
    bufferToStream(os, dev.m_addr, sizeof(dev.m_addr));
    return os;
}

}
