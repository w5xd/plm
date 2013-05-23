/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#ifndef MOD_W5XDINSTEON_POWERLINEMODULE_H
#define MOD_W5XDINSTEON_POWERLINEMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
#if defined (W5XDINSTEON_EXPORTS)
#define POWERLINE_DLL_ENTRY(type) __declspec(dllexport) type __stdcall
#else
#define POWERLINE_DLL_ENTRY(type) __declspec(dllimport) type __stdcall
#endif
#elif defined (LINUX32)
#define POWERLINE_DLL_ENTRY(type) extern type
#endif

    typedef struct Modem_t *Modem;
    typedef struct Dimm_t *Dimmer;
    typedef struct Keypad_t *Keypad;
    /* commPortName 
    ** on Windows, must be COMn where n is a number
    ** on Linux, must be /dev/tty___ where ___ is the full device name
    **
    ** returns a positive integer that must be used as a handle for
    ** "modem" in the remaining calls.
    */
    POWERLINE_DLL_ENTRY(Modem) openPowerLineModem(const char *commPortName, 
        int *wasOpen, int level, const char *logFileName);
    /* print the modem's link table */
    POWERLINE_DLL_ENTRY(int) printModemLinkTable(Modem modem);
    /* start device linking session as controller */
    POWERLINE_DLL_ENTRY(int) startLinking(Modem  modem, int group, int multiple);
    /* start device linking session as responder */
    POWERLINE_DLL_ENTRY(int) startLinkingR(Modem  modem, int group);
    /* cancel device linking session */
    POWERLINE_DLL_ENTRY(int) cancelLinking(Modem  modem);
    /* CLEAR modem link data. CAREFUL! */
    POWERLINE_DLL_ENTRY(int) clearModemLinkData(Modem  modem);
    /* after shutdownModem, the modem won't work until another open */
    POWERLINE_DLL_ENTRY(int) shutdownModem(Modem  modem);
    /* set to zero or less writes no messages. and 2 writes more than 1*/
    POWERLINE_DLL_ENTRY(int) setErrorLevel(Modem modem, int level);
    /* negative returns from other functions have values that can be looked up here */
    POWERLINE_DLL_ENTRY(const char *) getErrorMessage(int errNumber);
    /* get dimmer access by dimmer hardware address */
    POWERLINE_DLL_ENTRY(Dimmer) getDimmerAccess(Modem modem, const char *addr);
    /* given a dimmer, ask its setting. */
    POWERLINE_DLL_ENTRY(int) getDimmerValue(Dimmer dimmer, int fromCache);
    /* given a dimmer, ask it to start sending over its linking table. */
    POWERLINE_DLL_ENTRY(int) startGatherLinkTable(Dimmer dimmer);
    /* Have the PLM respond to the dimmer on its given group. */
    POWERLINE_DLL_ENTRY(int) linkAsController(Dimmer dimmer, int group);
    /* Remove PLM as as reponder to this dimmer on its group */
    POWERLINE_DLL_ENTRY(int) unLinkAsController(Dimmer dimmer, int group);
    /* given a dimmer, set it */
    POWERLINE_DLL_ENTRY(int) setDimmerValue(Dimmer dimmer, unsigned char v);
    /* given a dimmer, use FAST command (ramp rate = instant. v>0 is ON, v <= 0 OFF*/
    POWERLINE_DLL_ENTRY(int) setDimmerFast(Dimmer dimmer, int v);
    /* returns -1 if haven't startGatherLinkTable or timeout passes*/
    POWERLINE_DLL_ENTRY(int) getNumberOfLinks(Dimmer dimmer);
    /* print links to stderr*/
    POWERLINE_DLL_ENTRY(int) printLinkTable(Dimmer dimmer);
    /* given two insteon devices, link the second as a reponder to the first.
    ** The ls1, ls2, and ls3 values are usually important to the responder, but
    ** are added to the controller link table by this program as well*/
    POWERLINE_DLL_ENTRY(int) createDeviceLink(Dimmer controller, Dimmer responder, 
        unsigned char group, unsigned char ls1, unsigned char ls2, unsigned char ls3);
    /* Remove link entry from the controller for the group and, on the responder, 
    ** for the given value of ls3. If ls3 is specified as zero,
    ** then ALL responder links for the given group and controller are removed. */
    POWERLINE_DLL_ENTRY(int) removeDeviceLink(Dimmer controller, Dimmer responder, 
        unsigned char group, unsigned char ls3);
    /* returns number of links in the database */
    POWERLINE_DLL_ENTRY(int) getModemLinkRecords(Modem modem);
    /* all devices linked to this one, set them to this value*/
    POWERLINE_DLL_ENTRY(int) setAllDevices(Modem modem, unsigned char group, unsigned char v);
    /* return -1 on don't have modem all link, and -2 on no groups available */
    POWERLINE_DLL_ENTRY(int) getNextUnusedControlGroup(Modem m);
    /* for a given dimmer, create a linkage on the modem that matches the Dimmer's responders to its group 1 */
    /* return 0 on bad Dimmer, -1 on linkage not available for Dimmer, -2 on other error in process */
    /* otherwise returns number of links created */
    POWERLINE_DLL_ENTRY(int) createModemGroupToMatch(int group, Dimmer d);
    /* delete all links for group in controller, including those in devices it can reach */
    POWERLINE_DLL_ENTRY(int) deleteGroupLinks(Modem m, unsigned char group);
    /* given a dimmer, ask it to send its extended set response for a given button. 
    ** pBuf may be zero. Negative return means failed to talk to dimmer. 
    ** Otherwise returns the number of characters written to pBuf*/
    POWERLINE_DLL_ENTRY(int) extendedGet(Dimmer dimmer, unsigned char btn, unsigned char *pBuf, unsigned bufsize);
    POWERLINE_DLL_ENTRY(int) printExtendedGet(Dimmer dimmer, unsigned char btn);

    // range for houseCode is 'A' to 'P'. Range for unit is 1 to 16. If X10 is not set, then both are zero.
    POWERLINE_DLL_ENTRY(int) getX10Code(Dimmer dimmer, char *houseCode, unsigned char *unit);
    POWERLINE_DLL_ENTRY(int) setX10Code(Dimmer dimmer, char houseCode, unsigned char unit);

        /* get dimmer access by dimmer hardware address */
    POWERLINE_DLL_ENTRY(Keypad) getKeypadAccess(Modem modem, const char *addr);
    /* The button numbers are 1 through 8. For 6-key keypad, the
    ** #1 is the ON/OFF, A,B,C,D are 3,4,5,6. Unused are 2,7,8 */

    /* a FollowMask is... 
    **  The bit numbers: LSB is button 1 and MSB is button 8*/
    POWERLINE_DLL_ENTRY(int) setKeypadFollowMask(Keypad keypad, unsigned char button, unsigned char mask);
    /* an OffMask is ... 
    **  The bit numbers: LSB is button 1 and MSB is button 8*/
    POWERLINE_DLL_ENTRY(int) setKeypadOffMask(Keypad keypad, unsigned char button, unsigned char mask);
    POWERLINE_DLL_ENTRY(int) setWallLEDbrightness(Keypad keypad, unsigned char bright);
    POWERLINE_DLL_ENTRY(int) setNonToggleState(Keypad keypad, unsigned char button, unsigned char nonToggle);

#ifdef __cplusplus
}
#endif

#endif

