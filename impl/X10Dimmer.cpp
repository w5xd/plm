/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include "X10Dimmer.h"
#include "PlmMonitor.h"
#include "InsteonDevice.h"
namespace w5xdInsteon {
    
	int X10Dimmer::setDimmerValue(unsigned char charv) const
	{
		unsigned short unitMask = 1;
		unitMask <<= m_unit - 1;
		return m_plm->sendX10Command(m_houseCode, unitMask, charv != 0 ? X10_ON : X10_OFF);
	}
}


