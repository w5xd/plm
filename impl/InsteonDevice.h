/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITOR_INSTEONDEVICE_H
#define MOD_W5XDINSTEON_PLMMONITOR_INSTEONDEVICE_H
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
namespace w5xdInsteon {

class PlmMonitor;
class InsteonCommand;

class InsteonDeviceAddr
{   // class to represent the 3-byte device address and sort them.
public:
    InsteonDeviceAddr(){memset(&m_addr[0], 0xff, sizeof(m_addr));}
    InsteonDeviceAddr(const unsigned char *c)
    {        memcpy(m_addr, c, sizeof(m_addr));    }
    operator const unsigned char * ()const {return m_addr;}
    const unsigned char &operator  [](const int &i)const {return m_addr[i];}
    bool operator ==(const InsteonDeviceAddr &other)const
    { return memcmp(m_addr, other.m_addr, 3) == 0;}
    InsteonDeviceAddr & operator =(const unsigned char *p){memcpy(m_addr,p, sizeof(m_addr)); return *this;}
    bool operator <(const InsteonDeviceAddr &other) const;
    unsigned char   m_addr[3];
};

// operator to print device address
std::ostream & operator << (std::ostream &os, const InsteonDeviceAddr &dev);

class InsteonLinkEntry
{
public:
    static const unsigned char CONTROLLER_FLAG;
    InsteonLinkEntry(): m_flag(0), m_group(0), m_LinkSpecific1(0), m_LinkSpecific2(0), m_LinkSpecific3(0){}
    InsteonLinkEntry(const unsigned char *v)
    {
        m_flag = v[0];
        m_group =v[1];
        memcpy(&m_addr.m_addr[0], v+2, sizeof(m_addr.m_addr));
        m_LinkSpecific1 = v[5];
        m_LinkSpecific2 = v[6];
        m_LinkSpecific3 = v[7];
    }
    unsigned char m_flag;
    unsigned char m_group;
    InsteonDeviceAddr m_addr;
    unsigned char m_LinkSpecific1;
    unsigned char m_LinkSpecific2;
    unsigned char m_LinkSpecific3;

    void print(std::ostream &)const;
};

class InsteonDevice
{
public:
    enum {START_BYTE=0, MODEM_BYTE= 1, FROM_ADDR_START=2, TO_ADDR_START= 5, FLAG_BYTE= 8,
        COMMAND1=9, COMMAND2=10};
    enum {BROADCAST_BIT = 0x80, GROUP_BIT=0x40, ACK_BIT=0x20, EXTMSG_BIT=0x10};

    InsteonDevice(PlmMonitor *p, const unsigned char addr[3]);
    virtual ~InsteonDevice(){}
    bool operator < (const InsteonDevice &) const;
    virtual void incomingMessage(const std::vector<unsigned char> &, boost::shared_ptr<InsteonCommand>);

    int linkPlm(bool amController, unsigned char group, unsigned char ls1=2, unsigned char ls2=2, unsigned char ls3=2);
    int unLinkPlm(bool amController, unsigned char group, unsigned char ls3 = 0);
    int startGatherLinkTable();
    int numberOfLinks(int msecToWait = 5000)const;
    const char * printLinkTable() ;
    int extendedGet(unsigned char btn, unsigned char *pBuf, unsigned bufSize);
    const char * printExtendedGet(unsigned char btn);
    int createModemGroupToMatch(int group);
    bool linktableComplete()const{boost::mutex::scoped_lock l(m_mutex); return m_LinkTableComplete;}

    // remove links for isController true, or, for false, with matching ls3, or, if ls3 is zero, all values of ls3
    int removeLinks(const InsteonDeviceAddr &addr, unsigned char group, bool amController, unsigned char ls3);

    int createLink(InsteonDevice *responder, unsigned char group,
                               unsigned char ls1, unsigned char ls2, unsigned char ls3);
    int removeLink(InsteonDevice *responder, unsigned char group, unsigned char ls3);
    // returns 0 on success, houseCode is from 'A' to 'P', and unit is from 1 to 16.
    int getX10Code(char &houseCode, unsigned char &unit, unsigned char btn=1) const;


    const InsteonDeviceAddr &addr()const{return m_addr;}
    int getProductData();
protected:
    int createLinkWithModem(unsigned char group, bool amController, InsteonDevice *other, unsigned char og);
    int createLinkWithModem(unsigned char group, bool amController, 
                                       unsigned char ls1, unsigned char ls2, unsigned char ls3);
    int sendExtendedCommand(unsigned char button, unsigned char d2, unsigned char d3=0, unsigned char d4=0);
    int linkAddr(const InsteonDeviceAddr &dev, unsigned char grp, bool isControl, unsigned char ls3)const;
    bool amRespondingTo(const InsteonDeviceAddr &dev, unsigned char grp)const; 
    void reqAllLinkData(unsigned addr); // addr == 0 means all records. Otherwise just the record at that addr
    // same range of legal values as getX10Code
    int setX10Code(char houseCode, unsigned char unit, unsigned char btn=1);

    mutable boost::mutex    m_mutex;
    mutable boost::condition_variable   m_condition;
    PlmMonitor  *m_plm; // non ref-counted
    InsteonDeviceAddr m_addr;
    typedef std::map<unsigned, InsteonLinkEntry> LinkTable_t;
    LinkTable_t m_LinkTable;
    std::set<unsigned> m_UnusedLinks;
    bool m_requestedOneLink;
    unsigned m_finalAddr;
    unsigned m_lastAcqCommand1;
    std::string m_linkTablePrinted;
    std::map<unsigned char, std::string> m_extendedGetPrint;
    typedef std::map<unsigned char, std::vector<unsigned char> > ExtendedGetResults_t;
    ExtendedGetResults_t m_ExtendedGetResult;
    std::vector<unsigned char> m_productData;
    static const char X10HouseCodeToLetter[16];
    static const char X10HouseLetterToBits[16];//subtract 'A' from letter to index into this table
    static const char X10WheelCodeToBits[17]; // wheel codes 1 through 16 are valid
    static const char X10BitsToWheelCode[16]; // shift right 1 bit position to index into this table
    int m_incomingMessageCount;
private:
    bool m_LinkTableComplete;
};

// class for holding pointers to InsteonDevices sorted by their Device address.
class InsteonDevicePtr
{
public:
    InsteonDevicePtr(InsteonDevice *p) : m_p(p){} // takes ownership of p
    InsteonDevicePtr(boost::shared_ptr<InsteonDevice> p) : m_p(p){}
    InsteonDevicePtr(const unsigned char addr[3]): m_p(new InsteonDevice(0, addr)){}
    bool operator < (const InsteonDevicePtr &other) const {return *m_p < *other.m_p;}
public:
    boost::shared_ptr<InsteonDevice>    m_p;
};
}
#endif

