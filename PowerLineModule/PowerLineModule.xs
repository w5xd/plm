#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <../api/PowerLineModule.h>

#include "const-c.inc"

MODULE = PowerLineModule		PACKAGE = PowerLineModule		

INCLUDE: const-xs.inc

Dimmer
getDimmerAccess(modem, addr)
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
linkAsController(dimmer, group)
	Dimmer	dimmer
	int	group

int
startGatherLinkTable(dimmer)
	Dimmer	dimmer

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
getModemLinkRecords(modem)
	Modem modem

int
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
getNumberOfLinks(dimmer)
	Dimmer dimmer

int
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
