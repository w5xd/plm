/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include "PlmMonitorMap.h"
#include "PlmMonitor.h"
#include "Dimmer.h"

namespace w5xdInsteon {
/******************************************************************
** statics
*/
PlmMonitorMap::PlmMonitorMap_t *PlmMonitorMap::gPlmMonitorMap = 
    new PlmMonitorMap::PlmMonitorMap_t();

PlmMonitorMap::PlmIdMap_t *PlmMonitorMap::gPlmIdMap =
    new PlmMonitorMap::PlmIdMap_t();

std::mutex *gMutex = new std::mutex();

static const char * const gErrorMessages[] =
{
    "CreateFile failed",    // -1
    "SetCommState failed",
    "SetupComm failed",
    "SetupCommTimeouts failed",
    "WriteFile failed",     // -5
    "Write retry count exceeded", 
    "Command response read count exceeded",
    "Modem not opened",
    "Dimmer id unknown",
    "Dimmer value not known", // -10
    "Operation canceled",
    "Invalid X10 house code",
    "Invalid X10 unit code",
};

PlmMonitor *PlmMonitorMap::openPowerLineModem(const char *name, int *wasOpen, int level,
                                              const char *logFileName)
{
    std::unique_lock<std::mutex> l(*gMutex);
    if (wasOpen) *wasOpen = 1;
    PlmMonitorMap_t::iterator itor = gPlmMonitorMap->find(name);
    if (itor != gPlmMonitorMap->end()) 
        return itor->second.get();
    if (wasOpen) *wasOpen = 0;
    std::shared_ptr<PlmMonitor> p;
    p.reset(new PlmMonitor(name, logFileName));
    p->setErrorLevel(level);
    int retv = p->init();
    if (retv >= 0)
    {
        (*gPlmMonitorMap)[name] = p;
        (*gPlmIdMap)[p->which()] = p;
        return p.get();
    }
    else
        return 0;
}

std::shared_ptr<PlmMonitor> PlmMonitorMap::findOne(int which, bool remove)
{
    std::shared_ptr<PlmMonitor> p;
    {
        std::unique_lock<std::mutex> l(*gMutex);
        PlmIdMap_t::iterator found = gPlmIdMap->find(which);
        if (found != gPlmIdMap->end())
        {
            p = found->second;
            if (remove)
            {
                gPlmMonitorMap->erase(found->second->commPortName());
                gPlmIdMap->erase(found);
            }
        }
    }
    return p;
}

int PlmMonitorMap::shutdownModem(int which)
{
    std::shared_ptr<PlmMonitor> p = findOne(which, true);
    if (p) return p->shutdownModem();
    return -8;
}

int PlmMonitorMap::setErrorLevel(PlmMonitor *modem, int level)
{
    return modem->setErrorLevel(level);
}

const char * PlmMonitorMap::getErrorMessage(int m)
{
    int maxError = sizeof(gErrorMessages)/sizeof(gErrorMessages[0]);
    if (m >= 0)
        return "No error";
    int v = -m-1;
    if (v >= maxError)
        return "Invalid error code";
    return gErrorMessages[v];
}

}
