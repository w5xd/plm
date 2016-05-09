#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <../api/PowerLineModule.h>
#include <../api/X10Commands.h>

#include "const-c.inc"

MODULE = PowerLineModule		PACKAGE = PowerLineModule		

INCLUDE: const-xs.inc

Dimmer
getDimmerAccess(modem, addr)
	Modem	modem
	char *	addr

Fanlinc
getFanlincAccess(modem, addr)
	Modem	modem
	char *	addr

int
startLinking(modem, group, multiple)
	Modem	modem
	int	group
	int	multiple

int
startLinkingR(modem, group)
	Modem	modem
	int	group

int
cancelLinking(modem)
	Modem	modem

int
clearModemLinkData(modem)
	Modem	modem

int
getDimmerValue(dimmer, fromCache)
	Dimmer	dimmer
	int	fromCache

int
linkPlm(dimmer, amController, group, ls1, ls2, ls3)
	Dimmer	dimmer
	int 	amController
	unsigned char	group
	unsigned char 	ls1
	unsigned char	ls2
	unsigned char	ls3

int
unLinkPlm(dimmer, amController, group, ls3)
	Dimmer	dimmer
	int	amController
	unsigned char	group
	unsigned char 	ls3

int
startGatherLinkTable(dimmer)
	Dimmer	dimmer

int
truncateUnusedLinks(dimmer)
	Dimmer dimmer

const char *
getErrorMessage(errNumber)
	int	errNumber

Modem
openPowerLineModem(commPortName, level, logFileName)
	char *	commPortName
	int	level
	char *  logFileName
	INIT:
	    int wasOpen;
	PPCODE:
	    wasOpen = 0;
	    RETVAL = openPowerLineModem(commPortName, &wasOpen, level, logFileName);
	    XPUSHs(sv_2mortal(newSViv((IV)RETVAL))); /* will be [0] */
	    XPUSHs(sv_2mortal(newSViv(wasOpen)));   /* will be [1] */

int
printLogString(modem, s)
	Modem modem
	char *	s

void
monitor(modem, waitSecs) 
	Modem modem
	int waitSecs
	INIT:
           int ret = 0;
	   Dimmer dimmer = 0;
	   unsigned char group = 0;
	   unsigned char cmd1 = 0;
	   unsigned char cmd2 = 0 ;
	   unsigned char ls1 = 0;
	   unsigned char ls2 = 0;
	   unsigned char ls3 = 0;
	PPCODE:
	   ret = monitorModem(modem, waitSecs, &dimmer, &group, &cmd1, &cmd2, &ls1, &ls2, &ls3);
	   XPUSHs(sv_2mortal(newSViv(ret))); /* [0] */
	   if (ret > 0)
           {
		XPUSHs(sv_2mortal(newSViv((IV)dimmer))); /* [1] */
		XPUSHs(sv_2mortal(newSViv(group)));
		XPUSHs(sv_2mortal(newSViv(cmd1)));
		XPUSHs(sv_2mortal(newSViv(cmd2)));
		XPUSHs(sv_2mortal(newSViv(ls1)));
		XPUSHs(sv_2mortal(newSViv(ls2)));
		XPUSHs(sv_2mortal(newSViv(ls3)));	/* [7] */
           }

void
setMonitorState(modem, newState)
	Modem modem
	int newState

int
getModemLinkRecords(modem)
	Modem modem

int
getNextUnusedControlGroup(modem)
	Modem modem

int
setModemCommandDelay(modem, delayMsec)
	Modem modem
	int delayMsec

int
deleteGroupLinks(modem, group)
	Modem modem
	unsigned char group

int
createModemGroupToMatch(group, dimmer)
	int group
	Dimmer dimmer

const char *
printModemLinkTable(modem)
	Modem modem

int
setDimmerValue(dimmer, v)
	Dimmer	dimmer
	unsigned char	v

int
setDimmerFast(dimmer, v)
	Dimmer	dimmer
	int	v

int
setAllDevices(modem, g, v)
	Modem modem
	unsigned char	g
	unsigned char	v

int
sendX10Command(modem, houseCode, unitMask, x10Command)
	Modem modem
	char *houseCode
	unsigned unitMask
        unsigned x10Command
	PPCODE:
	    RETVAL = sendX10Command(modem, houseCode[0], unitMask, (enum X10Commands_t)x10Command);
            XPUSHs(sv_2mortal(newSViv(RETVAL)));

int
getNumberOfLinks(dimmer)
	Dimmer dimmer

const char *
printLinkTable(dimmer)
	Dimmer dimmer

int
setErrorLevel(modem, level)
	Modem	modem
	int	level

int
shutdownModem(modem)
	Modem	modem

int
createDeviceLink(controller, responder, group, ls1, ls2, ls3)
	Dimmer controller
	Dimmer responder
	unsigned char group
	unsigned char ls1
	unsigned char ls2
	unsigned char ls3

int
removeDeviceLink(controller, responder, group, ls3)
	Dimmer controller
	Dimmer responder
	unsigned char group
	unsigned char ls3

int
extendedGet(dimmer, btn)
	Dimmer dimmer
	unsigned char btn
	PPCODE:
		RETVAL = extendedGet(dimmer, btn, 0, 0);
                XPUSHs(sv_2mortal(newSViv(RETVAL)));

const char *
printExtendedGet(dimmer, btn)
	Dimmer dimmer
	unsigned char btn

int
getX10Code(dimmer)
	Dimmer dimmer
	INIT:
	    char houseCode=0;
	    unsigned char unit=0;
	PPCODE:
	    RETVAL = getX10Code(dimmer, &houseCode, &unit);
            XPUSHs(sv_2mortal(newSViv(RETVAL))); /* [0]*/
	    XPUSHs(sv_2mortal(newSVpvn(&houseCode, 1))); /* will be [1] */
	    XPUSHs(sv_2mortal(newSViv(unit)));   /* will be [2] */

int
setX10Code(dimmer, houseCode, unit)
	Dimmer dimmer
	char *houseCode
	unsigned char unit
	PPCODE:
		RETVAL = setX10Code(dimmer, houseCode[0], unit);
                XPUSHs(sv_2mortal(newSViv(RETVAL)));

int
setRampRate(dimmer, rate)
	Dimmer dimmer
	unsigned char rate

int
setOnLevel(dimmer, level)
	Dimmer dimmer
	unsigned char level

int
enterLinkMode(dimmer, group)
	Dimmer dimmer
	unsigned char group

Keypad
getKeypadAccess(modem, addr)
	Modem	modem
	char *	addr

int
setWallLEDbrightness(keypad, bright)
	Keypad keypad
	unsigned char bright

int
setKeypadFollowMask(keypad, button, mask)
	Keypad keypad
	unsigned char button
	unsigned char mask

int
setKeypadOffMask(keypad, button, mask)
	Keypad keypad
	unsigned char button
	unsigned char mask

int
setBtnRampRate(keypad, rate, button)
	Keypad keypad
	unsigned char rate
        unsigned char button

int
setBtnOnLevel(keypad, level, button)
	Keypad keypad
	unsigned char level
	unsigned char button

int
setNonToggleState(keypad, button, nonToggle)
	Keypad keypad
	unsigned char button
	unsigned char nonToggle

int getBtnX10Code(keypad, btn)
	Keypad keypad
	unsigned char btn
	INIT:
	    char houseCode=0;
	    unsigned char unit=0;
	PPCODE:
	    RETVAL = getBtnX10Code(keypad, &houseCode, &unit, btn);
            XPUSHs(sv_2mortal(newSViv(RETVAL))); /* [0]*/
	    XPUSHs(sv_2mortal(newSVpvn(&houseCode, 1))); /* will be [1] */
	    XPUSHs(sv_2mortal(newSViv(unit)));   /* will be [2] */
	

int setBtnX10Code(keypad, houseCode, unit, btn)
	Keypad keypad
	char *houseCode
	unsigned char unit
	unsigned char btn
	PPCODE:
		RETVAL = setBtnX10Code(keypad, houseCode[0], unit, btn);
                XPUSHs(sv_2mortal(newSViv(RETVAL)));

int setFanSpeed(fanlinc, speed)
	Fanlinc fanlinc
	unsigned char speed

int getFanSpeed(fanlinc)
	Fanlinc fanlinc

