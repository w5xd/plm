/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITORIMPL_H
#define MOD_W5XDINSTEON_PLMMONITORIMPL_H
#include <windows.h>
#include <string>
namespace w5xdInsteon {
class PlmMonitorWin
{
public:
        PlmMonitorWin(const char *commPortName, unsigned baudrate = 19200);
        ~PlmMonitorWin();
        int OpenCommPort();
        bool Read(unsigned char *, unsigned, unsigned *);
        bool Write(const unsigned char *, unsigned);
        const std::string &commPortName()const {return m_commPortName;}
protected:
        const std::string m_commPortName;
        HANDLE  m_CommPort;
        unsigned m_BaudRate;
};

class PlmMonitorIO : public PlmMonitorWin { public: PlmMonitorIO(const char *c, unsigned baudrate = 19200) : PlmMonitorWin(c, baudrate){}};
}
#endif
