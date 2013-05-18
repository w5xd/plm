/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITORIMPL_H
#define MOD_W5XDINSTEON_PLMMONITORIMPL_H
#include <string>
namespace w5xdInsteon {
    class PlmMonitorLinux
{
public:
        PlmMonitorLinux(const char *commPortName);
        ~PlmMonitorLinux();
        int OpenCommPort();
        bool Read(unsigned char *, unsigned, unsigned *);
        bool Write(unsigned char *, unsigned);
        const std::string &commPortName()const {return m_commPortName;}
protected:
        const std::string m_commPortName;
        int  m_CommPortFD;
};

class PlmMonitorIO : public PlmMonitorLinux { public: PlmMonitorIO(const char *c) : PlmMonitorLinux(c){}};
}
#endif

