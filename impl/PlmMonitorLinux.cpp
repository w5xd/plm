/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include "PlmMonitorLinux.h"

namespace w5xdInsteon {
PlmMonitorLinux::PlmMonitorLinux(const char *commPortName) : 
    m_commPortName(commPortName),
    m_CommPortFD(-1)
{}

PlmMonitorLinux::~PlmMonitorLinux()
{
    if (m_CommPortFD >= 0)
        ::close(m_CommPortFD);
}

int PlmMonitorLinux::OpenCommPort()
{
    if (m_CommPortFD >= 0)
        ::close(m_CommPortFD);
    m_CommPortFD = ::open(m_commPortName.c_str(), O_RDWR | O_NOCTTY);

    if (m_CommPortFD == -1)
        return -1;
    tcflush(m_CommPortFD, TCIFLUSH);

    return 0;
}

bool PlmMonitorLinux::Read(unsigned char *rbuf, unsigned sizeToRead, unsigned *nrr)
{
	struct termios newtio;
	tcgetattr(m_CommPortFD,&newtio);
	newtio.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNBRK | IGNPAR;
	newtio.c_oflag = ONLRET | ONOCR;
	newtio.c_lflag = 0;
	newtio.c_cc[VMIN] = 0;
	newtio.c_cc[VTIME] = 1;
    /*   MIN == 0; TIME > 0: TIME specifies the limit for a timer in tenths of a second. 
    The timer is started when read(2) is called. read(2) returns either when at least 
    one byte of data is available, or when the timer expires. If the timer expires 
    without any input becoming available, read(2) returns 0.     */
	tcsetattr(m_CommPortFD,TCSANOW,&newtio);
    
    int res = ::read(m_CommPortFD, rbuf, sizeToRead);
    if (res >= 0)
    	*nrr = res;
    return res >= 0;  // true is success
}

bool PlmMonitorLinux::Write(unsigned char *v, unsigned s)
{
    int res = ::write(m_CommPortFD, v, s);
    return true;
}
}
