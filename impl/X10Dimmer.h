/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITOR_X10DIMMER_H
#define MOD_W5XDINSTEON_PLMMONITOR_X10DIMMER_H
namespace w5xdInsteon {
	class PlmMonitor;
    class X10Dimmer 
    {
        /* This device is a single dimmer. It has one button, which is numbered 1
        ** The same class may be used to control a relay. Any nonzero setting is on and
        ** reports back as value 255. Relaylinc's also are controlled fine by this class,
        ** including the one with a sense wire. It acts pretty much like a wall switch
        ** from a software perspective.
        */
    public:
        X10Dimmer(PlmMonitor *plm, char houseCode, unsigned char unit)
			: m_houseCode(houseCode)
			, m_unit(unit)
			, m_plm(plm)
		{
		}
		bool operator < (const X10Dimmer &other) const
		{
			if (m_houseCode == other.m_houseCode)
				return m_unit < other.m_unit;
			return m_houseCode < other.m_houseCode;
		}
        int setDimmerValue(unsigned char) const;
    protected:
		const char m_houseCode;
		const unsigned char m_unit;
		PlmMonitor  *m_plm; // non ref-counted
    };

}
#endif

