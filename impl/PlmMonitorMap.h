/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_PLMMONITORMAP_H
#define MOD_W5XDINSTEON_PLMMONITORMAP_H
#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include "ExportDefinition.h"
namespace w5xdInsteon {
    class PlmMonitor;
    class W5XD_EXPORT PlmMonitorMap
    {
    public:
        static PlmMonitor *openPowerLineModem(const char *name, 
            int *wasOpen, int level, const char *logFileName = 0);
        static int setErrorLevel(PlmMonitor *, int);
        static int shutdownModem(int);
        static const char * getErrorMessage(int);

    private:
        typedef std::map<std::string, boost::shared_ptr<PlmMonitor> > PlmMonitorMap_t;
        static PlmMonitorMap_t *gPlmMonitorMap;
        typedef std::map<int, boost::shared_ptr<PlmMonitor> > PlmIdMap_t;
        static PlmIdMap_t *gPlmIdMap;
        static boost::shared_ptr<PlmMonitor> findOne(int, bool remove = false);
    };
}
#endif