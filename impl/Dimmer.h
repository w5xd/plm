/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITOR_DIMMER_H
#define MOD_W5XDINSTEON_PLMMONITOR_DIMMER_H
#include "InsteonDevice.h"
namespace w5xdInsteon {
    class InsteonCommand;
    class Dimmer : public InsteonDevice
    {
        /* This device is a single dimmer. It has one button, which is numbered 1
        ** The same class may be used to control a relay. Any nonzero setting is on and
        ** reports back as value 255. Relaylinc's also are controlled fine by this class,
        ** including the one with a sense wire. It acts pretty much like a wall switch
        ** from a software perspective.
        */
    public:
        Dimmer(PlmMonitor *plm, const unsigned char addr[3]);
        int getDimmerValue(bool fromCache) ;
        int setDimmerValue(unsigned char);
        int setDimmerFast(int);
        // houseCodes range from 'A' to 'P'. units range from 1 to 16
        // If none is set, then both houseCode and unit are zero
        int getX10Code(char &houseCode, unsigned char &unit) const { return InsteonDevice::getX10Code(houseCode, unit);}
        int setX10Code(char houseCode, unsigned char unit) {return InsteonDevice::setX10Code(houseCode, unit);}
        int setRampRate(unsigned char rate)  { return sendExtendedCommand(1, 5, rate); }
        int setOnLevel(unsigned char level)  { return sendExtendedCommand(1, 6, level);  }

        int createLink(InsteonDevice *controller, unsigned char controlGroup,
                               unsigned char brightness, unsigned char ramprate)
        { return controller->createLink(this, controlGroup, brightness, ramprate, 1); }
        // a dimmer responder doesn't care about ls3 values
        int removeLink(InsteonDevice *controller, unsigned char group){ return controller->removeLink(this, group, 0);}
    protected:
        void incomingMessage(const std::vector<unsigned char> &, std::shared_ptr<InsteonCommand>);
        bool    m_valueKnown;
        unsigned char m_lastKnown;
        int m_lastQueryCommandId;
    };
}
#endif

