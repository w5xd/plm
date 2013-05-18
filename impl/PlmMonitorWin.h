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
        PlmMonitorWin(const char *commPortName);
        ~PlmMonitorWin();
        int OpenCommPort();
        bool Read(unsigned char *, unsigned, unsigned *);
        bool Write(unsigned char *, unsigned);
        const std::string &commPortName()const {return m_commPortName;}
protected:
        const std::string m_commPortName;
        HANDLE  m_CommPort;
};

class PlmMonitorIO : public PlmMonitorWin { public: PlmMonitorIO(const char *c) : PlmMonitorWin(c){}};
}
#endif
