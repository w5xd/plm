/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITOR_H
#define MOD_W5XDINSTEON_PLMMONITOR_H
#include <string>
#include <vector>
#include <fstream>
#include <deque>
#include <set>
#include <map>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Dimmer.h"
#include "Keypad.h"

namespace w5xdInsteon {
    class PlmMonitorIO;

    extern void bufferToStream(std::ostream &st, const unsigned char *v, int s);

    class InsteonCommand
    {
    public:
        typedef boost::function1<void, InsteonCommand *> CompletionCb_t;
        InsteonCommand(unsigned rlen, int retry=1, CompletionCb_t fcn=0) :
          m_responseLength(rlen), m_answerState(0),m_timesSent(0), m_retry(retry), m_whenDone(fcn)
          {}
        std::vector<unsigned char>   m_command;
        const unsigned m_responseLength;
        boost::shared_ptr<std::vector<unsigned char> >  m_answer;
        int m_answerState;   // 0 means waiting. negative is error code. positive is finished OK
        int m_timesSent;
        int m_retry;
        int m_globalId;
        CompletionCb_t m_whenDone;
    };

    class PlmMonitor
    {
    public:
        PlmMonitor(const char *commPortName, const char *logFileName=0);
        ~PlmMonitor();
        int which() const { return m_which;}
        int init();
        const std::string &commPortName() const;

        int shutdownModem();
        int startLinking(int, bool);
        int startLinkingR(unsigned char);
        int getModemLinkRecords();
        int cancelLinking();
        int clearModemLinkData();
        int setAllDevices(unsigned char grp, unsigned char v);
        int nextUnusedControlGroup()const;
        const char * printModemLinkTable() ;

        template <class T>
        T *getDeviceAccess(const unsigned char addr[3]);
        template <class T>
        T *getDeviceAccess(const char *addr);

        // C callable dimmer--access by string name
        Dimmer *getDimmerAccess(const char *addr)
        {return getDeviceAccess<Dimmer>(addr);}
        Keypad *getKeypadAccess(const char *addr)
        {return getDeviceAccess<Keypad>(addr);}

        boost::shared_ptr<InsteonCommand> 
            queueCommand(const unsigned char *v, unsigned s, unsigned resLen, bool retry = true, InsteonCommand::CompletionCb_t fcn=0);
        boost::shared_ptr<InsteonCommand> 
            sendCommandAndWait(const unsigned char *v, unsigned s, unsigned resLen, bool retry = true);

        int createLink(InsteonDevice *who, bool amControl,
            unsigned char grp, unsigned char flag, unsigned char ls1, unsigned char ls2, unsigned char ls3);
        int removeLink(InsteonDevice *who, bool amControl, unsigned char grp);

        int setIfLinked(InsteonDevice *d, unsigned char v);
        int setFastIfLinked(InsteonDevice *d, int v);

        int deleteGroupLinks(unsigned char group);

        int monitor(int waitSeconds, InsteonDevice *&dimmer, 
            unsigned char &group, unsigned char &cmd1, unsigned char &cmd2,
            unsigned char &ls1, unsigned char &ls2, unsigned char &ls3);

        void queueNotifications(bool v);

        enum {MESSAGE_QUIET, MESSAGE_ON, MESSAGE_MOST_IO, MESSAGE_EVERY_IO};
        int m_verbosity;
        int setErrorLevel(int v) { int ret = m_verbosity; m_verbosity = v; return ret;}
        int getErrorLevel() const { return m_verbosity;}
        const unsigned char *insteonID()const{return m_IMInsteonId;}
        std::ostream &cerr(){return m_errorFile.is_open() ? m_errorFile : std::cerr;}
   protected:
        typedef std::set<InsteonDevicePtr> InsteonDeviceSet_t;
        struct NotificationEntry {
            boost::shared_ptr<InsteonDevice> m_device;
            unsigned char group;
            unsigned char cmd1;
            unsigned char cmd2;
            unsigned char ls1;
            unsigned char ls2;
            unsigned char ls3;
            boost::posix_time::ptime    m_received;
            NotificationEntry() : group(0), cmd1(0), cmd2(0), ls1(0), ls2(0), ls3(0){}
            bool operator == (const NotificationEntry &other)const;
        };
        mutable boost::mutex    m_mutex;
        std::ofstream   m_errorFile;
        boost::scoped_ptr<PlmMonitorIO> m_io;
        std::deque<boost::shared_ptr<InsteonCommand> > m_writeQueue;
        boost::condition_variable   m_condition;
        bool m_shutdown;
        int m_ThreadRunning;
        unsigned char   m_multipleLinkingRequested;
        unsigned char    m_IMInsteonId[3];
        unsigned char    m_IMDeviceCat;
        unsigned char    m_IMDeviceSubCat;
        unsigned char    m_IMFirmwareVersion;
        const int m_which;
        int m_nextCommandId;
        boost::posix_time::ptime    m_startupTime;
        InsteonDeviceSet_t  m_insteonDeviceSet;
        std::deque<InsteonDevicePtr>  m_retiredDevices;
        bool m_haveAllModemLinks;
        typedef std::vector<InsteonLinkEntry> LinkTable_t;
        LinkTable_t  m_ModemLinks;
        std::string m_linkTablePrinted;
        typedef std::deque<boost::shared_ptr<NotificationEntry> > NotificationEntryQueue_t;
        NotificationEntryQueue_t m_notifications;
        NotificationEntryQueue_t m_priorNotifications;
        bool m_queueNotifications;

        void startLinking(unsigned char);
        void ioThread();
        void syncWithThreadState(bool);
        void signalThreadStartStop(bool);
        int MessageGetImInfo();
        void timeStamp(std::ostream &st);
        void deliverfromRemoteMessage(boost::shared_ptr<std::vector<unsigned char> >, boost::shared_ptr<InsteonCommand>);
        void reportErrorState(const unsigned char *, int, boost::shared_ptr<InsteonCommand>);
        void getNextLinkRecordCompleted(InsteonCommand *);
        int getDeviceLinkGroup(InsteonDevice *d)const;
        void forceGetImInfo();
    };
}
#endif
