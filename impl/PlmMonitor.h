/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITOR_H
#define MOD_W5XDINSTEON_PLMMONITOR_H
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <deque>
#include <set>
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include "../api/X10Commands.h"

namespace w5xdInsteon {
    class Dimmer;
    class X10Dimmer;
    class X10DimmerCollection;
    class Keypad;
    class Fanlinc;
    class InsteonDevice;
    class PlmMonitorIO;
    class InsteonLinkEntry;

    extern void bufferToStream(std::ostream& st, const unsigned char* v, int s);

    // class for holding pointers to InsteonDevices sorted by their Device address.
    class InsteonDevicePtr
    {
    public:
        InsteonDevicePtr(InsteonDevice* p) : m_p(p) {} // takes ownership of p
        InsteonDevicePtr(std::shared_ptr<InsteonDevice> p) : m_p(p) {}
        InsteonDevicePtr(const unsigned char addr[3]);
        bool operator < (const InsteonDevicePtr& other) const;
    public:
        std::shared_ptr<InsteonDevice>    m_p;
    };


    class InsteonCommand
    {
    public:
        typedef std::function<void(InsteonCommand*)> CompletionCb_t;
        typedef std::function<void(InsteonCommand*, const std::vector<unsigned char>&)> OnNakCb_t;
        InsteonCommand(unsigned rlen, int retry = 1, CompletionCb_t fcn = CompletionCb_t(), OnNakCb_t OnNak = OnNakCb_t()) :
            m_responseLength(rlen), m_answerState(0), m_timesSent(0), m_retry(retry), m_whenDone(fcn)
            , m_timesNak(4), m_whenNAK(OnNak)
        {}
        std::vector<unsigned char>   m_command;
        unsigned m_responseLength;
        std::shared_ptr<std::vector<unsigned char> >  m_answer;
        int m_answerState;   // 0 means waiting. negative is error code. positive is finished OK
        int m_timesSent;
        int m_retry;
        int m_timesNak;
        int m_globalId;
        CompletionCb_t m_whenDone;
        OnNakCb_t m_whenNAK;
    };

    class PlmMonitor
    {
    public:
        PlmMonitor(const char* commPortName, const char* logFileName = 0);
        ~PlmMonitor();
        int which() const { return m_which; }
        int init();
        const std::string& commPortName() const;

        int shutdownModem();
        int startLinking(int, bool);
        int startLinkingR(unsigned char);
        int getModemLinkRecords();
        int cancelLinking();
        int clearModemLinkData();
        int setAllDevices(unsigned char grp, unsigned char v);
        int nextUnusedControlGroup()const;
        const char* printModemLinkTable();
        int printLogString(const char*);

        template <class T>
        T* getDeviceAccess(const unsigned char addr[3]);
        template <class T>
        T* getDeviceAccess(const char* addr);

        // C callable dimmer--access by string name
        Dimmer* getDimmerAccess(const char* addr)
        {
            return getDeviceAccess<Dimmer>(addr);
        }
        Keypad* getKeypadAccess(const char* addr)
        {
            return getDeviceAccess<Keypad>(addr);
        }
        Fanlinc* getFanlincAccess(const char* addr)
        {
            return getDeviceAccess<Fanlinc>(addr);
        }
        X10Dimmer* getX10DimmerAccess(char houseCode, unsigned char unit);

        int sendX10Command(char houseCode, unsigned short unitMask, enum X10Commands_t command);

        std::shared_ptr<InsteonCommand>
            queueCommand(const unsigned char* v, unsigned s, unsigned resLen, bool retry = true, InsteonCommand::CompletionCb_t fcn = InsteonCommand::CompletionCb_t(),
                InsteonCommand::OnNakCb_t onNak = InsteonCommand::OnNakCb_t());
        std::shared_ptr<InsteonCommand>
            sendCommandAndWait(const unsigned char* v, unsigned s, unsigned resLen, bool retry = true);

        int createLink(InsteonDevice* who, bool amControl,
            unsigned char grp, unsigned char flag, unsigned char ls1, unsigned char ls2, unsigned char ls3);
        int removeLink(InsteonDevice* who, bool amControl, unsigned char grp);

        int setIfLinked(InsteonDevice* d, unsigned char v);
        int setFastIfLinked(InsteonDevice* d, int v);

        int deleteGroupLinks(unsigned char group);

        int monitor(int waitSeconds, InsteonDevice*& dimmer,
            unsigned char& group, unsigned char& cmd1, unsigned char& cmd2,
            unsigned char& ls1, unsigned char& ls2, unsigned char& ls3);

        void queueNotifications(bool v);

        enum { MESSAGE_QUIET, MESSAGE_ON, MESSAGE_MOST_IO, MESSAGE_EVERY_IO };
        int m_verbosity;
        int setErrorLevel(int v) { int ret = m_verbosity; m_verbosity = v; return ret; }
        int getErrorLevel() const { return m_verbosity; }
        int setCommandDelay(int delayMsec);
        const unsigned char* insteonID()const { return m_IMInsteonId; }
        std::ostream& cerr() { return m_errorFile.is_open() ? m_errorFile : std::cerr; }
        static const unsigned char SET_ACQ_MSG_BYTE;
        static const unsigned char SET_ACQ_MSG_2BYTE;
        static const unsigned char SET_NAQ_MSG_BYTE;
    protected:
        typedef std::set<InsteonDevicePtr> InsteonDeviceSet_t;
        struct NotificationEntry {
            std::shared_ptr<InsteonDevice> m_device;
            unsigned char group;
            unsigned char cmd1;
            unsigned char cmd2;
            unsigned char ls1;
            unsigned char ls2;
            unsigned char ls3;
            std::chrono::steady_clock::time_point    m_received;
            NotificationEntry() : group(0), cmd1(0), cmd2(0), ls1(0), ls2(0), ls3(0) {}
            bool operator == (const NotificationEntry& other)const;
        };
        void NakCurrentMessage(const std::vector<unsigned char>& curMessage, int& delayQueue, std::deque<unsigned char>& leftOver, std::shared_ptr<InsteonCommand> p);
        mutable std::mutex    m_mutex;
        std::ofstream   m_errorFile;
        std::unique_ptr<PlmMonitorIO> m_io;
        std::deque<std::shared_ptr<InsteonCommand> > m_writeQueue;
        std::condition_variable   m_condition;
        bool m_shutdown;
        int m_ThreadRunning;
        unsigned char   m_multipleLinkingRequested;
        unsigned char    m_IMInsteonId[3];
        unsigned char    m_IMDeviceCat;
        unsigned char    m_IMDeviceSubCat;
        unsigned char    m_IMFirmwareVersion;
        unsigned int m_commandDelayMsec;
        const int m_which;
        int m_nextCommandId;
        std::chrono::steady_clock::time_point    m_startupTime;
        InsteonDeviceSet_t  m_insteonDeviceSet;

        std::deque<InsteonDevicePtr>  m_retiredDevices;
        std::thread m_thread;

        // X10 devices
        std::shared_ptr<X10DimmerCollection> m_X10Dimmers;

        bool m_haveAllModemLinks;
        typedef std::vector<InsteonLinkEntry> LinkTable_t;
        LinkTable_t  m_ModemLinks;
        std::string m_linkTablePrinted;
        typedef std::deque<std::shared_ptr<NotificationEntry> > NotificationEntryQueue_t;
        NotificationEntryQueue_t m_notifications;
        NotificationEntryQueue_t m_priorNotifications;
        bool m_queueNotifications;

        void startLinking(unsigned char);
        void ioThread();
        void syncWithThreadState(bool);
        void signalThreadStartStop(bool);
        int MessageGetImInfo();
        void timeStamp(std::ostream& st);
        void deliverfromRemoteMessage(std::shared_ptr<std::vector<unsigned char> >, std::shared_ptr<InsteonCommand>);
        void reportErrorState(const unsigned char*, int, const std::vector<unsigned char> &answer, std::shared_ptr<InsteonCommand>);
        void getNextLinkRecordCompleted(InsteonCommand*);
        void getLinkRecordNak(InsteonCommand*, const std::vector<unsigned char>&);
        int getDeviceLinkGroup(InsteonDevice* d)const;
        void forceGetImInfo();
        void writeCommand(std::shared_ptr<InsteonCommand>);
    };
}
#endif
