/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include "../api/PowerLineModule.h"
#include "../impl/PlmMonitorMap.h"
#include "../impl/PlmMonitor.h"
#include "../impl/Dimmer.h"
#include "../impl/Keypad.h"
#include "../impl/Fanlinc.h"
#include "../impl/X10Dimmer.h"

/***********************************************************************
**C callable entry points
*/

POWERLINE_DLL_ENTRY(Modem) openPowerLineModem(const char *name, int *wasOpen, int level,
                                              const char *logFileName)
{
    return reinterpret_cast<Modem>(w5xdInsteon::PlmMonitorMap::openPowerLineModem(name, 
        wasOpen, level, logFileName));
}

POWERLINE_DLL_ENTRY(int) printLogString(Modem m, const char *s)
{
	if (!m) return 0;
	return reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->printLogString(s);
}

POWERLINE_DLL_ENTRY(int) setErrorLevel(Modem m, int level)
{
    if (!m) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->setErrorLevel(level);
}

POWERLINE_DLL_ENTRY(int) startLinking(Modem  modem, int group, int multiple)
{
    if (!modem) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->startLinking(group, multiple != 0);
}

POWERLINE_DLL_ENTRY(int) startLinkingR(Modem  modem, int group)
{
    if (!modem) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->startLinkingR(static_cast<unsigned char>(group));
}

POWERLINE_DLL_ENTRY(const char *) printModemLinkTable(Modem  modem)
{
    if (!modem) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->printModemLinkTable();
}

POWERLINE_DLL_ENTRY(int) cancelLinking(Modem  modem)
{
    if (!modem) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->cancelLinking();
}

POWERLINE_DLL_ENTRY(int) clearModemLinkData(Modem  modem)
{
    if (!modem) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->clearModemLinkData();
}

POWERLINE_DLL_ENTRY(int) monitorModem(Modem modem, int waitSeconds, Dimmer *dimmer, 
        unsigned char *group, unsigned char *cmd1, unsigned char *cmd2,
        unsigned char *ls1, unsigned char *ls2, unsigned char *ls3)
{
    if (!modem) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->monitor(
        waitSeconds, *reinterpret_cast<w5xdInsteon::InsteonDevice ** >(dimmer), 
        *group, *cmd1, *cmd2, *ls1, *ls2, *ls3
        );
}

POWERLINE_DLL_ENTRY(void) setMonitorState(Modem modem, int state)
{
    if (!modem) return;
    reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->queueNotifications(state != 0);
}

POWERLINE_DLL_ENTRY(int) setModemCommandDelay(Modem modem, int delayMilliseconds)
{
    if (!modem) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor * >(modem)->setCommandDelay(delayMilliseconds);
}


POWERLINE_DLL_ENTRY(int) shutdownModem(Modem m)
{
    if (!m) return 0;
    return w5xdInsteon::PlmMonitorMap::shutdownModem(
        reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->which());
}

POWERLINE_DLL_ENTRY(const char *) getErrorMessage(int m)
{
    return w5xdInsteon::PlmMonitorMap::getErrorMessage(m);
}

POWERLINE_DLL_ENTRY(Dimmer) getDimmerAccess(Modem modem, const char *addr)
{
    if (!modem) return 0;
    return reinterpret_cast<Dimmer>(
        reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->getDimmerAccess(addr));
}

POWERLINE_DLL_ENTRY(Keypad) getKeypadAccess(Modem modem, const char *addr)
{
    if (!modem) return 0;
    return reinterpret_cast<Keypad>(
        reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->getKeypadAccess(addr));
}

POWERLINE_DLL_ENTRY(Fanlinc) getFanlincAccess(Modem modem, const char *addr)
{
    if (!modem) return 0;
    return reinterpret_cast<Fanlinc>(
        reinterpret_cast<w5xdInsteon::PlmMonitor *>(modem)->getFanlincAccess(addr));
}

POWERLINE_DLL_ENTRY(int) getDimmerValue(Dimmer dimmer, int fromCache)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->getDimmerValue(fromCache ? true : false);
}

POWERLINE_DLL_ENTRY(int) linkPlm(Dimmer dimmer, int amController, unsigned char group, unsigned char ls1, unsigned char ls2, unsigned char ls3)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->linkPlm(amController!=0, group, ls1, ls2, ls3);
}

POWERLINE_DLL_ENTRY(int) unLinkPlm(Dimmer dimmer, int amController, unsigned char group, unsigned char ls3)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->unLinkPlm(amController!=0, group, ls3);
}

POWERLINE_DLL_ENTRY(int) getNumberOfLinks(Dimmer dimmer)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::InsteonDevice *>(dimmer)->numberOfLinks();
}


POWERLINE_DLL_ENTRY(unsigned char) getInsteonEngineVersion(Dimmer dimmer)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::InsteonDevice*>(dimmer)->getInsteonEngineVersion();
}

POWERLINE_DLL_ENTRY(int) isLinkTableComplete(Dimmer dimmer)
{
    if (!dimmer) return -1;
    return reinterpret_cast<w5xdInsteon::InsteonDevice*>(dimmer)->linktableComplete() ? 1 : 0;
}

POWERLINE_DLL_ENTRY(void) pressSetButtonLink(Dimmer dimmer, unsigned char group) 
{
    if (!dimmer) return;
    return reinterpret_cast<w5xdInsteon::InsteonDevice*>(dimmer)->setLinkMode(group);
}

POWERLINE_DLL_ENTRY(void) pressSetButtonUnlink(Dimmer dimmer, unsigned char group)
{
    if (!dimmer) return;
    return reinterpret_cast<w5xdInsteon::InsteonDevice*>(dimmer)->setUnlinkMode(group);
}

POWERLINE_DLL_ENTRY(const char *) printLinkTable(Dimmer dimmer)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::InsteonDevice *>(dimmer)->printLinkTable();
}


POWERLINE_DLL_ENTRY(int) setDimmerValue(Dimmer dimmer, unsigned char v)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->setDimmerValue(v);
}

POWERLINE_DLL_ENTRY(int) setDimmerFast(Dimmer dimmer, int v)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->setDimmerFast(v);
}

POWERLINE_DLL_ENTRY(int) startGatherLinkTable(Dimmer dimmer)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::InsteonDevice *>(dimmer)->startGatherLinkTable();
}

POWERLINE_DLL_ENTRY(void) suppressLinkTableUpdate(Dimmer dimmer)
{
    if (!dimmer) return;
    return reinterpret_cast<w5xdInsteon::InsteonDevice*>(dimmer)->suppressLinkTableUpdate();
}

POWERLINE_DLL_ENTRY(int) createDeviceLink(Dimmer controller, Dimmer responder, 
        unsigned char group, unsigned char ls1, unsigned char ls2, unsigned char ls3)
{
    if (!controller || !responder) return 0;
    return reinterpret_cast<w5xdInsteon::InsteonDevice *>(controller)->createLink(
        reinterpret_cast<w5xdInsteon::InsteonDevice *>(responder), group,
        ls1, ls2, ls3);
}

POWERLINE_DLL_ENTRY(int) removeDeviceLink(Dimmer controller, Dimmer responder, 
        unsigned char group, unsigned char ls3)
{
    if (!controller || !responder) return 0;
    return reinterpret_cast<w5xdInsteon::InsteonDevice *>(controller)->removeLink(
        reinterpret_cast<w5xdInsteon::InsteonDevice *>(responder), group,
        ls3);
}

POWERLINE_DLL_ENTRY(int) truncateUnusedLinks(Dimmer dimmer)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::InsteonDevice *>(dimmer)->truncateUnusedLinks();
}

POWERLINE_DLL_ENTRY(int) getModemLinkRecords(Modem m)
{
    if (!m) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->getModemLinkRecords();
}

POWERLINE_DLL_ENTRY(int) setAllDevices(Modem m, unsigned char grp, unsigned char v)
{
    if (!m) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->setAllDevices(grp, v);
}

POWERLINE_DLL_ENTRY(int) getNextUnusedControlGroup(Modem m)
{
    if (!m) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->nextUnusedControlGroup();
}

POWERLINE_DLL_ENTRY(int) createModemGroupToMatch(int group, Dimmer d)
{
    if (!d) return 0;
    return reinterpret_cast<w5xdInsteon::InsteonDevice *>(d)->createModemGroupToMatch(group);
}

POWERLINE_DLL_ENTRY(int) deleteGroupLinks(Modem m, unsigned char group)
{
    if (!m) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->deleteGroupLinks(group);
}

POWERLINE_DLL_ENTRY(int) extendedGet(Dimmer dimmer, unsigned char btn, unsigned char *pBuf, unsigned bufsize)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->extendedGet(btn, pBuf, bufsize);
}

POWERLINE_DLL_ENTRY(const char *) printExtendedGet(Dimmer dimmer, unsigned char btn)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->printExtendedGet(btn);
}

POWERLINE_DLL_ENTRY(int) getX10Code(Dimmer dimmer, char *houseCode, unsigned char *unit)
{
    if (!dimmer || !houseCode || !unit) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->getX10Code(*houseCode, *unit);
}

POWERLINE_DLL_ENTRY(int) setX10Code(Dimmer dimmer, char houseCode, unsigned char unit)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->setX10Code(houseCode, unit);
}

POWERLINE_DLL_ENTRY(int) setRampRate(Dimmer dimmer, unsigned char rate)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->setRampRate(rate);
}

POWERLINE_DLL_ENTRY(int) setOnLevel(Dimmer dimmer, unsigned char level)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::Dimmer *>(dimmer)->setOnLevel(level);
}

POWERLINE_DLL_ENTRY(int) setKeypadFollowMask(Keypad keypad, unsigned char button, unsigned char mask)
{
    if (!keypad) return 0;
    return reinterpret_cast<w5xdInsteon::Keypad *>(keypad)->setKeypadFollowMask( button, mask);
}

POWERLINE_DLL_ENTRY(int) setKeypadOffMask(Keypad keypad, unsigned char button, unsigned char mask)
{
    if (!keypad) return 0;
    return reinterpret_cast<w5xdInsteon::Keypad *>(keypad)->setKeypadOffMask( button, mask);
}

POWERLINE_DLL_ENTRY(int) setNonToggleState(Keypad keypad, unsigned char button, unsigned char nonToggle)
{
    if (!keypad) return 0;
    return reinterpret_cast<w5xdInsteon::Keypad *>(keypad)->setNonToggleState(button, nonToggle);
}

POWERLINE_DLL_ENTRY(int) setWallLEDbrightness(Keypad keypad, unsigned char bright)
{
    if (!keypad) return 0;
    return reinterpret_cast<w5xdInsteon::Keypad *>(keypad)->setWallLEDbrightness(bright);
}

POWERLINE_DLL_ENTRY(int) getBtnX10Code(Keypad keypad, char *houseCode, unsigned char *unit, unsigned char btn)
{
    if (!keypad || !houseCode || !unit) return 0;
    return reinterpret_cast<w5xdInsteon::Keypad *>(keypad)->getX10Code(*houseCode, *unit, btn);
}

POWERLINE_DLL_ENTRY(int) setBtnX10Code(Keypad keypad, unsigned char houseCodeBin, unsigned char unitBin, unsigned char btn)
{
    if (!keypad) return 0;
    return reinterpret_cast<w5xdInsteon::Keypad *>(keypad)->setX10Code(houseCodeBin, unitBin, btn);
}

POWERLINE_DLL_ENTRY(int) setBtnRampRate(Keypad keypad, unsigned char rate, unsigned char btn)
{
    if (!keypad) return 0;
    return reinterpret_cast<w5xdInsteon::Keypad *>(keypad)->setRampRate(rate, btn);
}

POWERLINE_DLL_ENTRY(int) setBtnOnLevel(Keypad keypad, unsigned char level, unsigned char btn)
{
    if (!keypad) return 0;
    return reinterpret_cast<w5xdInsteon::Keypad *>(keypad)->setOnLevel(level, btn);
}

POWERLINE_DLL_ENTRY(int) setFanSpeed(Fanlinc fanlinc, unsigned char speed)
{
    if (!fanlinc)
        return 0;
    return reinterpret_cast<w5xdInsteon::Fanlinc *>(fanlinc)->setFanSpeed(speed);
}

POWERLINE_DLL_ENTRY(int) getFanSpeed(Fanlinc fanlinc)
{
    if (!fanlinc)
        return -1;
    return reinterpret_cast<w5xdInsteon::Fanlinc *>(fanlinc)->getFanSpeed();
}

POWERLINE_DLL_ENTRY(int) sendX10Command(Modem m, char houseCode, unsigned short unitMask, enum X10Commands_t command)
{
    if (!m) return 0;
    return reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->sendX10Command(houseCode, unitMask, command);
}

POWERLINE_DLL_ENTRY(X10Dimmer) getX10DimmerAccess(Modem m, char houseCode, unsigned char unit)
{
    if (!m) return 0;
    return reinterpret_cast<X10Dimmer>(
		reinterpret_cast<w5xdInsteon::PlmMonitor *>(m)->getX10DimmerAccess(houseCode, unit));
}

POWERLINE_DLL_ENTRY(int) setX10DimmerValue(X10Dimmer dimmer, unsigned char v)
{
    if (!dimmer) return 0;
    return reinterpret_cast<w5xdInsteon::X10Dimmer *>(dimmer)->setDimmerValue(v);
}
