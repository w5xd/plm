/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include <iomanip>
#include <sstream>
#include <fstream>
#include <boost/bind.hpp>
#include "PlmMonitor.h"
#include "Dimmer.h"
#include "X10Dimmer.h"
#include "Fanlinc.h"
#include "Keypad.h"

#if defined (WIN32)
#include "PlmMonitorWin.h"
#elif defined (LINUX32)
#include "PlmMonitorLinux.h"
#endif

#define DIM(x) (sizeof(x) / sizeof((x)[0]))

namespace w5xdInsteon {

static int gNextId = 10349;
enum {MAX_COMMAND_READS = 5, MAX_RETRIES = 8};
static const int COMMAND_DELAY_MSEC = 1000;
const unsigned char PlmMonitor::SET_ACQ_MSG_BYTE = 0x68;
const unsigned char PlmMonitor::SET_ACQ_MSG_2BYTE = 0x71;
const unsigned char PlmMonitor::SET_NAQ_MSG_BYTE = 0x70;

static const unsigned char OUT_OF_ORDER[] =
{
    PlmMonitor::SET_ACQ_MSG_BYTE,
    PlmMonitor::SET_ACQ_MSG_2BYTE,
    PlmMonitor::SET_NAQ_MSG_BYTE
};

static bool isOutOfOrder(unsigned char v)
{
    for (int i = 0 ; i < DIM(OUT_OF_ORDER); i++)
        if (v == OUT_OF_ORDER[i]) return true;
    return false;
}

void bufferToStream(std::ostream &st, const unsigned char *v, int s)
{
    st << std::hex ;
    for (int i = 0; i < s; i++)
    {
        if (i > 0) st << " ";
        st << "0x" << std::setfill('0') << std::setw(2) << (int)*v++;
    }
}

namespace {
	struct compPtr {
			bool operator()(const boost::shared_ptr<X10Dimmer> &A,
				const boost::shared_ptr<X10Dimmer> &B) const
			{
				if (!A || !B)
					return A < B;
				return *A.get() < *B.get();
			}
		};
}

class X10DimmerCollection : public std::set<boost::shared_ptr<X10Dimmer>, compPtr> {};

void PlmMonitor::timeStamp(std::ostream &st)
{
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
    std::streamsize p = st.precision(6);
    st << now << " (" << std::setfill(' ') << std::setw(13) << std::fixed << std::showpoint  
        << (now-m_startupTime).total_microseconds() / 1000000.0 << ") ";
    st.precision(p);
}

/**********************************************************************
** member functions
*/

PlmMonitor::PlmMonitor(const char *commPortName, const char *logFileName) : 
    m_which(gNextId++), 
    m_shutdown(false),
    m_ThreadRunning(0), 
    m_io(new PlmMonitorIO(commPortName)),
    m_verbosity(static_cast<int>(MESSAGE_QUIET)),
    m_startupTime(boost::posix_time::microsec_clock::universal_time()),
    m_multipleLinkingRequested(0),
    m_haveAllModemLinks(false), 
    m_nextCommandId(0),
    m_queueNotifications(false),
    m_commandDelayMsec(COMMAND_DELAY_MSEC),
	m_X10Dimmers(new X10DimmerCollection())
{
    if (logFileName && *logFileName)
        m_errorFile.open(logFileName, std::ofstream::out | std::ofstream::app);
}

PlmMonitor::~PlmMonitor()
{
    m_shutdown = true;
    syncWithThreadState(false);
}

int PlmMonitor::printLogString(const char *s)
{
    if (s) cerr() << s;
    return 1;
}

const std::string &PlmMonitor::commPortName() const {return m_io->commPortName();}

void PlmMonitor::syncWithThreadState(bool v)
{
    boost::mutex::scoped_lock l(m_mutex);
    while (v ^ (m_ThreadRunning != 0))
        m_condition.wait(l);
}

void PlmMonitor::signalThreadStartStop(bool v)
{
    {
        boost::mutex::scoped_lock l(m_mutex);
        m_ThreadRunning += v ? 1 : -1;
    }
    m_condition.notify_all();
}

int PlmMonitor::shutdownModem()
{
    m_shutdown = true;
    m_condition.notify_all();
    return 0;
}

void PlmMonitor::reportErrorState(const unsigned char *command, int clen,
                                  boost::shared_ptr<InsteonCommand> p)
{
    if (p->m_answerState < 0)
    {
        if (m_verbosity >= static_cast<int>(MESSAGE_ON))
        {
            timeStamp(cerr());
            cerr() << "PlmMonitor::sendCommandAndWait error on ";
            bufferToStream(cerr(), command, clen);
            cerr() << " answer ";
            if (!p->m_answer->empty())
                bufferToStream(cerr(), &(*p->m_answer)[0], static_cast<int>(p->m_answer->size()));
            cerr() << " answer_state: " << std::dec << p->m_answerState << std::endl;
        }
    }
}

boost::shared_ptr<InsteonCommand> PlmMonitor::queueCommand(
    const unsigned char *v, unsigned s, unsigned resLen, bool retry, InsteonCommand::CompletionCb_t fcn)
{
    boost::shared_ptr<InsteonCommand> p;
    p.reset(new InsteonCommand(resLen, retry ? MAX_RETRIES : 0, fcn));
    p->m_command.assign(v, v+s);
    {
        boost::mutex::scoped_lock l(m_mutex);
        p->m_globalId = ++m_nextCommandId;
        if (m_writeQueue.empty() || (s <= 1) || !isOutOfOrder(v[1]))
            m_writeQueue.push_back(p);
        else
        {
            // scoot this particular command ahead of anything that is not the same command
            std::deque<boost::shared_ptr<InsteonCommand> >::reverse_iterator itor = m_writeQueue.rbegin();
            while (itor != m_writeQueue.rend() && 
                    ((*itor)->m_command.size() > 1) &&
                    !isOutOfOrder((*itor)->m_command[1]))
                    itor++;
            m_writeQueue.insert(itor.base(), p);
        }
    }

    if (m_verbosity >= static_cast<int>(MESSAGE_EVERY_IO))
    {
        timeStamp(cerr());
        cerr() << "PlmMonitor::queueCommand " ;
        bufferToStream(cerr(), &p->m_command[0], static_cast<int>(p->m_command.size()));
        cerr() << std::endl;
    }
    return p;
}
        
boost::shared_ptr<InsteonCommand> 
PlmMonitor::sendCommandAndWait(const unsigned char *v, unsigned s, unsigned resLen, bool retry)
{
    boost::shared_ptr<InsteonCommand> p = queueCommand(v, s, resLen, retry);
    {
        boost::mutex::scoped_lock l(m_mutex);
        // wait for an answer
        while (p->m_answerState == 0)
            m_condition.wait(l);
    }
    reportErrorState(v, s, p);
    return p;
}

void
PlmMonitor::forceGetImInfo()
{
    static const unsigned char buf[] =  {0x02, 0x60};
    {
        boost::mutex::scoped_lock l(m_mutex);
        if (
            m_writeQueue.empty() ||
            m_writeQueue.front()->m_command.size() != sizeof(buf) ||
            memcmp(buf, &m_writeQueue.front()->m_command[0], sizeof(buf)) != 0)
        {   // only push this if the first command is not already GetImInfo
            boost::shared_ptr<InsteonCommand> p;
            p.reset(new InsteonCommand(9));
            p->m_command.assign(buf, buf + sizeof(buf));
            p->m_globalId = ++m_nextCommandId;
            m_writeQueue.push_front(p);
        }
    }
}

int PlmMonitor::MessageGetImInfo()
{
    // Get IM Info message
    int ret = 0;
    static const unsigned char buf[] =  {0x02, 0x60};
    boost::shared_ptr<InsteonCommand> p = sendCommandAndWait(buf, sizeof(buf), 9);
    if (p->m_answerState < 0)
        return p->m_answerState;

    if ((*p->m_answer)[8] == 0x6)
    {
        memcpy(m_IMInsteonId, &(*p->m_answer)[2], sizeof(m_IMInsteonId));
        m_IMDeviceCat = (*p->m_answer)[5];
        m_IMDeviceSubCat = (*p->m_answer)[6];
        m_IMFirmwareVersion = (*p->m_answer)[7];
        ret = p->m_timesSent;
        if (m_verbosity >= static_cast<int>(MESSAGE_ON))
        {
            cerr() << "Plm modem id = ";
            bufferToStream(cerr(), &(*p->m_answer)[2], 3);
            cerr() << std::endl;
        }
    }
    else
        return -1;

    return ret;
}

int PlmMonitor::init()
{
    int status = m_io->OpenCommPort();
    if (status < 0) return status;
    boost::thread   t(boost::bind(&PlmMonitor::ioThread, this));
    syncWithThreadState(true);
    {  // Set Modem mode 
        static const unsigned char tempWrite[] = {0x02, 0x6b, 0x0};   
        sendCommandAndWait(tempWrite, sizeof(tempWrite), 4, false);
        static const unsigned char getImConfig[]  = {0x02, 0x73};
        sendCommandAndWait(getImConfig, sizeof(getImConfig), 6, false);
    }
    MessageGetImInfo();
    return status;
}

static const char FirstIMCommand = 0x50;
static const char LastIMCommand = 0x58;
static const int ReceiveCommandSizes[] =
{
  11,  //0x50  Standard Message Received
  25,  //0x51  Extended Message Received
   4,  //0x52  X10 Received
  10,  //0x53  ALL-Linking completed
   3, //0x54  Button Event Report
   2, //0x55  User Reset Detected
   7, //0x56  All-Link Cleanup Failure Report
  10, //0x57  All link record response
   3, //0x58  all-link cleanup status report
};

static const int LargestMessageLength = 25;
static const int ReopenDeviceAfterFails = 100;

void PlmMonitor::ioThread()
{
    signalThreadStartStop(true);
    boost::shared_ptr<InsteonCommand> p;
    boost::shared_ptr<InsteonCommand> lastCommandAcqed;
    boost::posix_time::ptime timeOfLastIncomingMessage = boost::posix_time::microsec_clock::universal_time();
    unsigned char rbuf[100];
    int sizeToRead = LargestMessageLength + 1;
    std::deque<unsigned char> leftOver;
    int readError = 0;
    int delayDequeue = 0;
    int failureMessageRepeatCount = 0;
    while (!m_shutdown)
    {
        boost::shared_ptr<std::vector<unsigned char> > msg (
            new std::vector<unsigned char>(leftOver.begin(), leftOver.end()));
        std::vector<unsigned char> &curMessage(*msg.get());
        if (m_verbosity >= static_cast<int>(MESSAGE_MOST_IO))
        {
            if (!leftOver.empty())
            {
                timeStamp(cerr());
                cerr() << "leftOver: ";
                std::vector<unsigned char> b(leftOver.begin(), leftOver.end());
                bufferToStream(cerr(), &b[0], static_cast<int>(b.size()));
                cerr() << std::endl;
            }
        }
        leftOver.clear();
        // read next "message"
        // A "message" is defined to be a string of bytes that:
        //  start with 0x02
        //  next byte is either matches the corresponding byte in the last command
        //      we sent it, or
        //  is in the range FirstIMCommand to LastIMCommand
        //  Is of the length specified by the IM command or Host command
        //
        // If we reach a 500msec timeout without any of the above happening,
        // then discard what we have.
        //
        // if we receive more characters than we need, then leave them in leftOver
        // looking for command responses is more tolerant that just polling for input
        int currentCommandReadLoopCount = p ? MAX_COMMAND_READS : 0;
        for (;;)
        {
            unsigned nr=0;
            if (m_io->Read(rbuf, sizeToRead, &nr))
                readError = 0;
            else if (++readError >= ReopenDeviceAfterFails)
            {
                readError = 0;
                int status = m_io->OpenCommPort();
                if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                {
                   if (status >= 0)
                   {
                       cerr() << "Reopened comm port successfully" << std::endl;
                       failureMessageRepeatCount = 0;
                   }
                   else
                   {
                       if (failureMessageRepeatCount++ == 0)
                            cerr() << "Reopened comm port and failed again" << std::endl;
                   }
                }
                if (status < 0)
                {   // on failures, there was no wait in the Read call
                    boost::xtime xt;
                    boost::xtime_get(&xt, boost::TIME_UTC_);
                    xt.sec += 2;
                    boost::thread::sleep(xt);
                }
                else
                {
                    if (p)
                    {
                        m_writeQueue.push_front(p);
                        p.reset();
                    }
                    forceGetImInfo();
                }
            }
            if (nr != 0)    // if we got anything, ensure we go around again
                 currentCommandReadLoopCount = 1;
            for (unsigned i = 0; i < nr; i++)
                curMessage.push_back(rbuf[i]);

            if (m_verbosity >= static_cast<int>(MESSAGE_EVERY_IO))
            {
                if ((nr > 0) || (curMessage.size() > 0))
                {
                    timeStamp(cerr());
                    cerr() << "PlmMonitor::ioThread read" << std::endl;
                    bufferToStream(cerr(), rbuf, nr);
                    if (!curMessage.empty())
                    {
                        cerr() << std::endl << " with current message:" << std::endl;
                        bufferToStream(cerr(), &curMessage[0], static_cast<int>(curMessage.size()));
                    }
                    else
                        cerr() << std::endl << " with no current message";
                    cerr() << std::endl;
                }
            }
            
            // message must start with 0x02
            int charsDiscarded = 0;
            unsigned lastCharDiscarded = 0;
            while ((curMessage.size() > 0) && (curMessage[0] != 0x02))
            {
                charsDiscarded += 1;
                lastCharDiscarded = *curMessage.begin();
                curMessage.erase(curMessage.begin()); // discard until in sync
            }

            if (charsDiscarded && (m_verbosity >= static_cast<int>(MESSAGE_ON)))
                cerr() << "PlmMonitor::ioThread discarded " << std::dec << charsDiscarded << " characters ending with 0x" << 
                std::hex << lastCharDiscarded << std::endl;

            if (curMessage.size() >= 2)
            {
                unsigned char msgID = curMessage[1];
                unsigned expectedCount = 0;
                if ((msgID >= FirstIMCommand) && (msgID <= LastIMCommand))
                    expectedCount = ReceiveCommandSizes[msgID - FirstIMCommand];
                else if (p && (msgID == p->m_command[1]))
                    expectedCount = p->m_responseLength;
                else    // discard first character and try again
                    curMessage.erase(curMessage.begin());
                // put away left-overs
                while (curMessage.size() > expectedCount)
                {
                    leftOver.push_front(curMessage.back());
                    curMessage.resize(curMessage.size() - 1);
                }

                if (curMessage.size() == expectedCount)
                    break; // successful read of IM->Host

                sizeToRead = static_cast<int>(expectedCount - curMessage.size());
            }
            else
                sizeToRead -= nr;

            if (--currentCommandReadLoopCount < 0)
            {   // we read for a while and failed to receive what we wanted/expected
                if (p)
                {   // have a command outstanding and received nothing, 
                    if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                    {
                        timeStamp(cerr());
                        cerr() << "PlmMonitor::ioThread loop count discard for message:" << std::endl;
                        bufferToStream(cerr(), &p->m_command[0], static_cast<int>(p->m_command.size()));
                        cerr() << " with answer ";
                        if (!curMessage.empty())
                            bufferToStream(cerr(), &curMessage[0], static_cast<int>(curMessage.size()));
                        else cerr() << "empty.";
                        cerr() << " Reopening tty port" << std::endl;
                    }
                    m_io->OpenCommPort();
                    if (--p->m_retry < 0)
                    {
                        if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                           cerr() << "Retry count exceeded" << std::endl;
                        // terminate this command--not getting an answer
                        p->m_answer = msg;
                        {
                            boost::mutex::scoped_lock l(m_mutex);
                            p->m_answerState = -7;
                            m_condition.notify_all();
                        }
                        if (p->m_whenDone) p->m_whenDone(p.get());
                    }
                    else
                    {
                        if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                           cerr() << "Retrying after reopening TTY" << std::endl;
                        {
                            boost::mutex::scoped_lock l(m_mutex);
                            //put it back in queue at the front
                            m_writeQueue.push_front(p);
                            delayDequeue = 5; // don't dequeue this command for a bit
                        }
                        forceGetImInfo();
                    }
                    p.reset();
               }
               if (curMessage.size())
                   if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                       cerr() << "Discarding " << std::dec << curMessage.size() << " characters." << std::endl;
               curMessage.clear();
               break;   // nothing here, but check for incoming command
            }
        }

        sizeToRead = LargestMessageLength + 1;

        if (!curMessage.empty())
        {
            if (p &&  // we have sent a command....
                  (curMessage.size() > 1) &&
                     (p->m_command[1] == curMessage[1]))
            {   // successful command completion
                bool print = m_verbosity >= static_cast<int>(MESSAGE_EVERY_IO);
                if ((p->m_command.size() > 1) &&
                    (p->m_command[1] == 0x73) &&
                    (m_verbosity >= static_cast<int>(MESSAGE_ON)))
                    print = true;
                if (print)
                {
                    timeStamp(cerr());
                    cerr() << "PlmMonitor::ioThread successful completion for" << std::endl;
                    bufferToStream(cerr(), &p->m_command[0], static_cast<int>(p->m_command.size()));
                    cerr() << " with answer" << std::endl;
                    bufferToStream(cerr(), &curMessage[0], static_cast<int>(curMessage.size()));
                    cerr() << std::endl;
                }
                p->m_answer = msg;

                unsigned char acq = (*p->m_answer)[p->m_responseLength-1];

                if (((p->m_answer->size() >= p->m_responseLength) && 
                    (acq == 0x6)) ||
                    (--p->m_retry < 0))
                {
                    {
                        boost::mutex::scoped_lock l(m_mutex);
                        if (acq == 0x6)
                        {
                        	p->m_answerState = 1;
                            lastCommandAcqed = p;
                        }
                        else if (p->m_retry < 0) 
                            p->m_answerState = -1;
                        else 
                            p->m_answerState = -9;
                        m_condition.notify_all();
                    }
                    if (p->m_whenDone)  p->m_whenDone(p.get());
                    p.reset();
                }
                else 
                {   // answer is not adequate
                    if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                    {
                        timeStamp(cerr());
                        cerr() << "PlmMonitor::ioThread retrying command " << std::endl;
                        bufferToStream(cerr(), &p->m_command[0], static_cast<int>(p->m_command.size()));
                        cerr() << std::endl;
                    }
                    boost::mutex::scoped_lock l(m_mutex);
                    //put it back in queue at the front
                    m_writeQueue.push_front(p);
                    p.reset();
                    delayDequeue = 10;
                }
            }
            else        
            {   // is not an answer to our command
		        if (m_verbosity >= static_cast<int>(MESSAGE_ON))
		        {
                    timeStamp(cerr());
                    cerr() << "Delivering remote message" << std::endl;
			        bufferToStream(cerr(), &(*msg)[0], static_cast<int>(msg->size()));
			        cerr() << std::endl;
		        }
                timeOfLastIncomingMessage = boost::posix_time::microsec_clock::universal_time();
                if (p)
                {   // delay it.
                    boost::mutex::scoped_lock l(m_mutex);
                    m_writeQueue.push_front(p);
                    p.reset();
                    if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                        cerr() << "Delaying previously dequeued p" << std::endl;
                }
                deliverfromRemoteMessage(msg, lastCommandAcqed);
                curMessage.clear();
            }
        }
        if (curMessage.empty() && !p)    // only look for commands if nothing pending from modem
        {
            if (leftOver.empty() && (--delayDequeue <= 0)) 
            {// check for commands to write to modem
                boost::mutex::scoped_lock l(m_mutex);
                if (!m_writeQueue.empty())
                {
                    boost::shared_ptr<InsteonCommand> temp = m_writeQueue.front();
                    boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
                    if (
                        (temp->m_command.size() < 2) ||
                        // commands 0x62 and 0x63 cannot be sent until m_commandDelayMsec after the last thing we heard from the PLM
                        ((temp->m_command[1] != 0x62) && (temp->m_command[1] != 0x63)) ||
                        ((now - timeOfLastIncomingMessage).total_milliseconds() > m_commandDelayMsec)
                        )
                    {
                        m_writeQueue.pop_front();
                        p = temp;
                    }
                }
                delayDequeue = 0;
            }
            if (p)
            {
                timeOfLastIncomingMessage = boost::posix_time::microsec_clock::universal_time();
                writeCommand(p);
            }
        }
        if (p)
            sizeToRead = p->m_responseLength;
    }
    {
        boost::mutex::scoped_lock l(m_mutex);
        while (!m_writeQueue.empty())
        {
            p = m_writeQueue.front();
            m_writeQueue.pop_front();
            p->m_answerState = -11;
        }
        m_condition.notify_all();
    }
    signalThreadStartStop(false);
    if (m_verbosity >= static_cast<int>(MESSAGE_ON))
    {
        timeStamp(cerr());
        cerr() << "PlmMonitor::ioThread exit" << std::endl;
    }
}

void PlmMonitor::deliverfromRemoteMessage(boost::shared_ptr<std::vector<unsigned char> > v,
                                          boost::shared_ptr<InsteonCommand> lastAcqued)
{
    std::vector<unsigned char> &raw = *v.get();
    switch (raw[1])
    {
    case 0x50:
    case 0x51:
        if (raw.size() >= 11)
        {
            unsigned char addr[3];
            addr[0] = raw[2]; addr[1] = raw[3]; addr[2] = raw[4];
            boost::shared_ptr<InsteonDevice> p;
            {
                boost::mutex::scoped_lock l(m_mutex);
                InsteonDeviceSet_t::iterator itor = m_insteonDeviceSet.find(addr);
                if (itor != m_insteonDeviceSet.end())
                    p = itor->m_p;
            }
            if (p)
            {
                p->incomingMessage(raw, lastAcqued);
                unsigned char flag = raw[8] & 0xf0;
                if (raw[1] == 0x50) 
                {
                    unsigned char group = raw[7];
                    unsigned char cmd1 = raw[9];
                    unsigned char cmd2 = raw[10];
                    bool doNotify = (flag == 0xc0) && (raw[5] == 0) && (raw[6] == 0);
                    if (flag == 0x40)
                    {   // P2P Cleanup
                        if (memcmp(&m_IMInsteonId[0], &raw[5], sizeof(m_IMInsteonId)) == 0)
                        {
                            // addressed to me
                            doNotify = true;
                            group = cmd2;
                        }
                    }
                    if (doNotify)
                    {
                        boost::mutex::scoped_lock l(m_mutex);
                        if (m_queueNotifications)
                        {
                            boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
                            // put together a NotificationEntry for this message
                            boost::shared_ptr<NotificationEntry> nep (new NotificationEntry());
                            NotificationEntry &ne = *nep.get();
                            ne.m_device = p;
                            ne.group = group;
                            ne.cmd1 = cmd1;
                            ne.cmd2 = cmd2;
                            ne.m_received = now;
                            for (LinkTable_t::iterator itor = m_ModemLinks.begin();
                                itor != m_ModemLinks.end();
                                itor ++)
                            {
                                if ((itor->m_addr == p->addr()) &&
                                    (itor->m_group == ne.group) &&
                                    (itor->isResponder()))
                                {
                                    ne.ls1 = itor->m_LinkSpecific1;
                                    ne.ls2 = itor->m_LinkSpecific2;
                                    ne.ls3 = itor->m_LinkSpecific3;
                                    break;
                                }
                            }

                            bool duplicate = false;
                            for (NotificationEntryQueue_t::iterator itor = m_priorNotifications.begin();
                                    itor != m_priorNotifications.end();
                                    )
                            {
                                if ((now - itor->get()->m_received) > 
                                    boost::posix_time::time_duration(0, 0, 1))
                                {   // anything old in the notification queue is deleted
                                    itor = m_priorNotifications.erase(itor);
                                }
                                else
                                {   // anything not too told in the queue is compared with the new one
                                    if (*itor->get() == ne)
                                        duplicate = true;
                                    itor++;
                                }
                            }
                            m_priorNotifications.push_back(nep);
                            if (!duplicate)
                            {
                                m_notifications.push_back(nep);
                                m_condition.notify_all();
                            }
                        }
                        else
                            m_priorNotifications.clear();
                    }
                }
            }
        }
        break;
    case 0x57:
        if (m_verbosity >= static_cast<int>(MESSAGE_ON))
        {
            cerr() << "Modem all-link record is ";
            if (!v->empty()) bufferToStream(cerr(), &(*v)[0], static_cast<int>(v->size()));
            cerr() << std::endl;
        }
        if (raw.size() >= 10)
        {
            boost::mutex::scoped_lock l(m_mutex);
            m_ModemLinks.push_back(InsteonLinkEntry(&raw[2]));
        }
        else
            if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                cerr() << "Received rejected Link data message because size = " << raw.size() << std::endl;
        break;
    case 0x53:
        if (raw.size() == 10)
        {
            if (m_verbosity >= static_cast<int>(MESSAGE_ON))
            {
                cerr() << "Device link received from ";
                bufferToStream(cerr(), &(*v)[4], 3);
                cerr() << std::endl;
            }
            unsigned char backToLinking = 0;
            {
                boost::mutex::scoped_lock l(m_mutex);
                backToLinking = m_multipleLinkingRequested;
            }
            if (backToLinking != 0)
            {
                if (m_verbosity >= static_cast<int>(MESSAGE_ON))
                    cerr() << "Back to linking\n" << std::endl;
                startLinking(backToLinking);
            }
        }
        break;
    case 0x56:
        if (raw.size() >= 7)
        {
            if (m_verbosity >= static_cast<int>(MESSAGE_ON))
            {
                cerr() << "All-Linked-Cleanup failure report ";
                bufferToStream(cerr(), &raw[2], 2);
                cerr() << " addr ";
                bufferToStream(cerr(), &raw[4], 3);
                cerr() << std::endl;
            }
        }
    case 0x58:
        if (raw.size() >= 3)
        {
            if (m_verbosity >= static_cast<int>(MESSAGE_ON))
            {
                cerr() << "All-Linked-Cleanup status ";
                bufferToStream(cerr(), &raw[2], 1);
                cerr() << std::endl;
            }
        }
        break;
    }
}

void PlmMonitor::writeCommand(boost::shared_ptr<InsteonCommand>p)
{
    p->m_timesSent += 1;
    if (m_verbosity >= static_cast<int>(MESSAGE_ON))
    {
        timeStamp(cerr());
        cerr() << "Writing command " << std::dec << p->m_globalId << " ";
        bufferToStream(cerr(), &p->m_command[0], static_cast<int>(p->m_command.size()));
        cerr() << std::endl;
    }
    if (!m_io->Write(&p->m_command[0], static_cast<int>(p->m_command.size())))
    {   // fail now if failed to write
        {
            boost::mutex::scoped_lock l(m_mutex);
            p->m_answerState = -5;
            m_condition.notify_all();
        }
        if (p->m_whenDone) p->m_whenDone(p.get());
        p.reset();
        if (m_verbosity >= static_cast<int>(MESSAGE_ON))
        {
            timeStamp(cerr());
            cerr() << "Write failed" << std::endl;
        }
    }
}

void PlmMonitor::startLinking(unsigned char group)
{
        unsigned char buf[4] =  {0x02, 0x64, 0x01};
        buf[3] = group;
        queueCommand(buf, sizeof(buf), 5);
}

int PlmMonitor::startLinkingR(unsigned char group)
{
    unsigned char buf[4] =  {0x02, 0x64, 0};
    buf[3] = group;
    queueCommand(buf, sizeof(buf), 5);
    return 1;
}

template <class T>
T *PlmMonitor::getDeviceAccess(const char *p)
{
    unsigned char addr[3];
    memset(addr, 0, sizeof(addr));
    int i;
    for (i = 0; i < 3; i++)
    {
        if (!p || !*p)
            break;
        while (*p && *p != '.')
        {
            addr[i] <<= 4;
            unsigned char c = toupper(*p);
            if ((c >= 'A') && (c <= 'F'))
                addr[i] += c - 'A' + 10;
            else if (isdigit(c))
                addr[i] += c - '0';
            p += 1;
        }
        if (*p == '.') p++;
    }
    if (i < 3)
        return 0;
    return getDeviceAccess<T>(addr);
}

// access by insteon address
template <class T>
T *PlmMonitor::getDeviceAccess(const unsigned char addr[3])
{
    boost::mutex::scoped_lock l(m_mutex);
    InsteonDeviceSet_t::iterator itor = m_insteonDeviceSet.find(addr);
    if (itor != m_insteonDeviceSet.end())
    {
        T *current = dynamic_cast<T *>(itor->m_p.get());
        if (current)
            return current;
        else 
        {
            m_retiredDevices.push_back(*itor); // can't just delete--C clients might still use
            m_insteonDeviceSet.erase(itor);
        }
    }
    boost::shared_ptr<InsteonDevice> v(new T(this, addr));
    m_insteonDeviceSet.insert(InsteonDevicePtr(v));
    return dynamic_cast<T *>(v.get());
}

X10Dimmer *PlmMonitor::getX10DimmerAccess(char houseCode, unsigned char wheelCode)
{
    boost::mutex::scoped_lock l(m_mutex);
	boost::shared_ptr<X10Dimmer> dimmer(new X10Dimmer(this, houseCode, wheelCode));
	X10DimmerCollection::iterator itor = m_X10Dimmers->find(dimmer);
	if (itor == m_X10Dimmers->end())
		m_X10Dimmers->insert(dimmer);
	else
		dimmer = *itor;
	return dimmer.get();
}

int PlmMonitor::startLinking(int group, bool multiple)
{
    startLinking((unsigned char)group);
    {
        boost::mutex::scoped_lock l(m_mutex);
        m_multipleLinkingRequested = multiple ? (unsigned char)group : 0;
    }
    return 0;
}

int PlmMonitor::cancelLinking()
{
    static unsigned char buf[] =  {0x02, 0x65};
    boost::shared_ptr<InsteonCommand> p = sendCommandAndWait(buf, sizeof(buf), 3);
    return 0;
}

int PlmMonitor::getModemLinkRecords()
{
    int ret = 0;
    {
        boost::mutex::scoped_lock l(m_mutex);
        m_haveAllModemLinks = false;
        m_ModemLinks.clear();
    }
    {   // request first all-link record
        static const unsigned char reqAllLink[] =
        {0x02, 0x69};
        boost::shared_ptr<InsteonCommand> q = sendCommandAndWait(reqAllLink, sizeof(reqAllLink), 3, false);
        reportErrorState(reqAllLink, sizeof(reqAllLink), q);
        getNextLinkRecordCompleted(q.get());
        boost::mutex::scoped_lock l(m_mutex);
        while (!m_haveAllModemLinks)
            m_condition.wait(l);
        ret = static_cast<int>(m_ModemLinks.size());
    }
    return ret;
}

void PlmMonitor::getNextLinkRecordCompleted(InsteonCommand *q)
{
    if (q->m_answer && (q->m_answer->size() > 2) && q->m_answer->at(2) == 0x6)
    {
        static const unsigned char reqNextLink[] =
        {0x02, 0x6A};
        queueCommand(reqNextLink, sizeof(reqNextLink), 3, false, 
            boost::bind(&PlmMonitor::getNextLinkRecordCompleted, this, _1));
    }
    else
    {
        boost::mutex::scoped_lock l(m_mutex);
        m_haveAllModemLinks = true;
        m_condition.notify_all();
    }
}

int PlmMonitor::clearModemLinkData()
{
    static unsigned char buf[] =  {0x02, 0x67};
    boost::shared_ptr<InsteonCommand> p = sendCommandAndWait(buf, sizeof(buf), 3);
    boost::mutex::scoped_lock l(m_mutex);
    m_haveAllModemLinks = true;
    m_ModemLinks.clear();
    return 0;
}

int PlmMonitor::nextUnusedControlGroup() const
{
    {
        boost::mutex::scoped_lock l(m_mutex);
        if (!m_haveAllModemLinks)
            return -1;
    }
    int grpNum = 2;
    while (grpNum < 254)
    {
        bool found = false;
        for (unsigned i = 0; i < m_ModemLinks.size(); i++)
        {
            if ((m_ModemLinks[i].m_flag & 0x40) && (grpNum == m_ModemLinks[i].m_group))
            {
                found = true;
                grpNum += 1;
                break;
            }
        }
        if (!found) break;
    }
    if (grpNum < 254)
        return grpNum;
    return -2;
}

int PlmMonitor::createLink(InsteonDevice *who, bool amControl, unsigned char grp, 
                           unsigned char flag, unsigned char ls1, unsigned char ls2, unsigned char ls3)
{
    unsigned char buf[] =  {0x02, 0x6F, amControl ? 0x40 : 0x41, flag, grp, 
        who->addr()[0], who->addr()[1], who->addr()[2], ls1, ls2, ls3};
    boost::shared_ptr<InsteonCommand> p = sendCommandAndWait(buf, sizeof(buf), 12);
    return p->m_answerState;
}

int PlmMonitor::removeLink(InsteonDevice *who, bool amControl, unsigned char grp)
{
    const InsteonDeviceAddr &addr = who->addr();
    unsigned char buf[11] = {
        0x02, 0x6f, 0x80, amControl ? 0xe2 : 0, grp, 
            addr[0], addr[1], addr[2],
            0, 0, 0};
     boost::shared_ptr<InsteonCommand> p = sendCommandAndWait(buf, sizeof(buf), 12);
     return p->m_answerState;
}

int PlmMonitor::deleteGroupLinks(unsigned char group)
{
    // removes all links for which I am the controller of the group
    std::vector<InsteonDevice *> devicesInvolved;
    std::vector<InsteonLinkEntry> toDelete;
    {
        LinkTable_t LinkTableCopy;
        {
            boost::mutex::scoped_lock l(m_mutex);
            if (!m_haveAllModemLinks)
                return -1;
            LinkTableCopy = m_ModemLinks;
        }
        for (LinkTable_t::iterator itor = LinkTableCopy.begin(); itor != LinkTableCopy.end(); itor++)
        {
            if (itor->m_flag & 0x40) // is controller
            {
                if (itor->m_group == group)
                {
                    devicesInvolved.push_back(getDeviceAccess<InsteonDevice>(itor->m_addr));
                    toDelete.push_back(*itor);
                }
            }
        }
    }

    InsteonDeviceSet_t insteonDeviceSet;
    {
        boost::mutex::scoped_lock l(m_mutex);
        insteonDeviceSet = m_insteonDeviceSet;
    }

    for (
        InsteonDeviceSet_t::iterator itor =  insteonDeviceSet.begin();
        itor != insteonDeviceSet.end();
        itor ++)
        {
            if (itor->m_p->numberOfLinks(10) > 0)
            {
                // if we are not already planning to deal with this device...
                bool found = false;
                for (unsigned i = 0; i < devicesInvolved.size(); i++)
                {
                    if (devicesInvolved[i]->addr() == itor->m_p->addr())
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    devicesInvolved.push_back(itor->m_p.get());
            }
        }

    for (unsigned i = 0; i < devicesInvolved.size(); i++)
    {
        InsteonDevice *dev = devicesInvolved[i];
        dev->startGatherLinkTable();
        if (dev->numberOfLinks() < 0)
            return -2;
        int res = dev->removeLinks(m_IMInsteonId, group, false, 0);
        if (res < 0)
            return -3;
    }

    {
        boost::mutex::scoped_lock l(m_mutex);
        m_haveAllModemLinks = false;
    }

    for (unsigned i = 0; i < toDelete.size(); i++)
    {
        InsteonLinkEntry &le = toDelete[i];
        unsigned char buf[11] = {
            0x02, 0x6f, 0x80, le.m_flag, le.m_group, 
                le.m_addr[0], le.m_addr[1], le.m_addr[2],
                le.m_LinkSpecific1, le.m_LinkSpecific2, le.m_LinkSpecific3};

         boost::shared_ptr<InsteonCommand> p = sendCommandAndWait(buf, sizeof(buf), 12);
        if (p->m_answerState < 0)
            return p->m_answerState;
    }

    return 0;
}

int PlmMonitor::getDeviceLinkGroup(InsteonDevice *d) const
{
    int group = -1;
    {
        boost::mutex::scoped_lock l(m_mutex);
        if (!m_haveAllModemLinks)
            return -1;
        for (LinkTable_t::const_iterator itor = m_ModemLinks.begin(); itor != m_ModemLinks.end(); itor ++)
        {
            const InsteonLinkEntry &link = *itor;
            if (link.m_addr == d->addr())
            {
                if (link.m_flag & 0x40)    // I am controller
                {
                    if ((link.m_LinkSpecific1 == 0) &&
                        (link.m_LinkSpecific2 == 0) &&
                        (link.m_LinkSpecific3 == 1)) // Values we put here when we created this link
                    {
                        group = link.m_group;
                        break;
                    }
                }
            }
        }
    }
    return group;
}

int PlmMonitor::setAllDevices(unsigned char grp, unsigned char v)
{
    unsigned char buf[5] = {0x02, 0x61, grp, v, 0};
    queueCommand(buf, sizeof(buf), 6);
    return 0;
}

int PlmMonitor::setIfLinked(InsteonDevice *d, unsigned char v)
{
    int group = getDeviceLinkGroup(d);
    if (group > 0)
        return setAllDevices(static_cast<unsigned char>(group), v != 0 ? 0x11 : 0x13);
    return -1;
}

int PlmMonitor::setFastIfLinked(InsteonDevice *d, int v)
{
    int group = getDeviceLinkGroup(d);
    if (group > 0)
        return setAllDevices(static_cast<unsigned char>(group), v > 0 ? 0x12 : 0x14);
    return -1;
}

int PlmMonitor::sendX10Command(char houseCode, unsigned short unitMask, enum X10Commands_t command)
{
	// X10 protocol:
	// have to send a message to select each unit
	unsigned char buf[4] = {0x02, 0x63, 0, 0};
	unsigned char hc = InsteonDevice::X10HouseLetterToBits[(houseCode - 'A') & 0xF] << 4;;
	unsigned char mask = 1;
	for (int i = 0; i < 16; i++)
	{
		if (unitMask & mask)
		{
			buf[2] = (InsteonDevice::X10WheelCodeToBits[i+1] >> 1) | hc;
			queueCommand(buf, sizeof(buf), 5);
		}
		mask <<= 1;
	}
	buf[3] = 0x80;
	buf[2] = hc | static_cast<unsigned char>(command);
	queueCommand(buf, sizeof(buf), 5);
	return 1;
}

const char * PlmMonitor::printModemLinkTable() 
{
    boost::mutex::scoped_lock l(m_mutex);
    if (!m_haveAllModemLinks) 
    {
        cerr() << "Modem links not retrieved yet" << std::endl;
        return 0;
    }
    std::ostringstream linkTablePrinted;
    linkTablePrinted << "Modem links" << std::endl << "flag group   ID       ls1 ls2 ls3" << std::endl;
    for (LinkTable_t::const_iterator itor = m_ModemLinks.begin();
            itor != m_ModemLinks.end();
            itor++)
    {
        itor->print(linkTablePrinted);
        linkTablePrinted << std::endl;
    }
    m_linkTablePrinted = linkTablePrinted.str(); 
    return m_linkTablePrinted.c_str();
}

int PlmMonitor::monitor(int waitSeconds, InsteonDevice *&dimmer, 
            unsigned char &group, unsigned char &cmd1, unsigned char &cmd2,
            unsigned char &ls1, unsigned char &ls2, unsigned char &ls3)
{
    boost::mutex::scoped_lock l(m_mutex);
    if (waitSeconds < 0)
        m_queueNotifications = true;
    m_ThreadRunning += 1;
    while (!m_shutdown && m_queueNotifications && m_notifications.empty())
    {
        if (waitSeconds < 0)
            m_condition.wait(l);
        else if (waitSeconds > 0)
        {
            m_condition.timed_wait(l, boost::posix_time::time_duration(0, 0, waitSeconds));
            break;
        }
        else
            break;
    }
    m_ThreadRunning -= 1;
    if (m_notifications.empty())
        return 0;
    NotificationEntry &ne = *m_notifications.front().get();
    dimmer = ne.m_device.get();
    group = ne.group;
    cmd1 = ne.cmd1;
    cmd2 = ne.cmd2;
    ls1 = ne.ls1;
    ls2 = ne.ls2;
    ls3 = ne.ls3;
    m_notifications.pop_front();
    return 1;
}

void PlmMonitor::queueNotifications(bool v)
{
    {
        boost::mutex::scoped_lock l(m_mutex);
        m_queueNotifications = v;
        if (!v)
        {
            m_notifications.clear();
            m_priorNotifications.clear();
        }
    }
    m_condition.notify_all();
}

bool PlmMonitor::NotificationEntry::operator == (const NotificationEntry &other) const
{
    if (*m_device.get()->addr() != *other.m_device.get()->addr())
        return false;
    if (group != other.group)
        return false;
    if (other.cmd1 != cmd1)
        return false;
    if (other.cmd2 != cmd2)
        return false;
    return true;
}

int PlmMonitor::setCommandDelay(int delayMsec)
{
    if ((delayMsec >= 10) && (delayMsec <= 10000))
        m_commandDelayMsec = delayMsec;
    return (int) m_commandDelayMsec;
}

InsteonDevicePtr::InsteonDevicePtr(const unsigned char addr[3]): m_p(new InsteonDevice(0, addr)){}

bool InsteonDevicePtr::operator < (const InsteonDevicePtr &other) const {return *m_p < *other.m_p;}

// force a couple of template instantiations. These are currently used above, but show how to make more, if needed
template Dimmer *PlmMonitor::getDeviceAccess<Dimmer>(const char *);
template Keypad *PlmMonitor::getDeviceAccess<Keypad>(const char *);
template Fanlinc *PlmMonitor::getDeviceAccess<Fanlinc>(const char *);
template InsteonDevice *PlmMonitor::getDeviceAccess<InsteonDevice>(const char *);


}

