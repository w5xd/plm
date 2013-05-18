/* Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
** See license at http://github.com/w5xd/plm/blob/master/LICENSE.md */
#include <stdio.h>
#if defined (WIN32)
#include <windows.h>
#define SLEEP(X) Sleep((X) * 1000)
#elif defined (LINUX32)
#include <unistd.h>
#define SLEEP(X) sleep(X)
#endif
#include <PowerLineModule.h>

static void DimmerTest(Dimmer dimmer);

/*
** How to use
**
** This is partially a test program, but also partially a utility.
**
** Set the dimmer at 11.11.11 to value 0. The -w is usually needed to give the commands time
** to work before exit.
** PlmTest -d 11.11.11 -s 0 -w 2 COM3
**
** If you have cross-linked insteon devices such that switching the "real" device
** changes an indicator on some other device, then you probably would like the
** dimmer commands from your PLM to also manipulate that other device.
** This is accomplished by using this utility (once) to setup links among the
** "main" device, any devices it is linked to as a controller, and this PLM.
**
** For example: these set up group 2 on your PLM to control 11.11.11
** and links every device that 11.11.11 controls to match:
** First delete any group 2 on your PLM:
** PlmTest -X -g 2
** Then make group 2 control that device
** PlmTest -x -d 11.11.11 -g 2 COM3
**
** The link tables are in flash in the PLM (and the other devices) such that
** subsequent attempts to turn on/off 11.11.11 using setDimmerValue use
** the group rather than direct-to-device commands. Only values of 0 and 255
** are routed through the group. All other values are direct.
*/

int main (int argc, char **argv)
{
    const char *modName = "";
    const char *dimmerAddr = 0;
    Dimmer dimmer=0;
    Modem m = 0;
    int reset = 0;
    int cmdStartLink = 0; /* >0 is link as control, <0 is responder*/
    int linkGroup = -1;
    int i;
    int setVal = -1;
    int waitSeconds = 2;
    int createLinks = 0;
    int deleteLinks = 0;
    int dimmerTest = 0;
    int allStuff = 0;
    int printModemLinks = 0;
    int printDimmerLinks = 0;
    int printExtRecords = 0;
    const char *responderLink = 0;
    const char *responderUnlink = 0;
    int X10Unit = -1;
    char X10Hc = 0;
    int X10 = 0;
    int ls1 = -1, ls2 = -1, ls3 = -1;
    if (argc < 2)
    {
        fprintf(stderr, "Usage: PlmTest [-r] [-l] [-g grp] [-d x.y.z] [-s value] [-w [tmo]] [-x] [-X] <ComPort>\n");
        fprintf(stderr, 
            " -r       Reset modem to factory defaults. All other commands ignored\n"
            " -l       Start linking process as controller on group grp (default to 254)\n"
            " -L       Start linking process  as responder on group grp.\n"
            "          If -d not specified, then stay in link mode for 4 minutes and exit\n"
            " -p       Print modem link table\n"
            " -g grp   Set group number for -s and -l to grp\n"
            " -d x.y.z Dimmer operations on Insteon address x,y.z\n"
            "          If also -l or -L, then command dimmer to appropriate link mode\n"
            " -d pl    Print modem link table. Can be combined with another -d\n"
            " -s value Command group grp or dimmer to on or off\n"
            "          -1 with -d runs a dimmer test sequence\n"
            "          Any other negative value just retrieves dimmer value\n"
            " -w tmo   Wait for tmo seconds (default to 30) and print monitored traffic\n"
            " -x       For the dimmer -d, cross link records on this modem to duplicate\n"
            "          its group 1 behavior when turned on/off from this modem\n"
            "          Specify a group number, -g, otherwise will use next unused group number in the PLM\n"
            " -X       for the group -g, track down all links on this modem and on devices and delete them\n"
            " -X10 [hc unit] for the dimmer -d,\n"
            "          print its X10 house code if hc unit not specified. Otherwise set X10 house code and unit.\n"
            "          A zero unit clears the X10 code in the device\n"
            " -e [n]   For specified dimmer, print n extended records.\n"
            " -R <x.y.z> <ls1> <ls2> <ls3>\n"
            "          Links -d as controller, x.y.z as responder with values ls1, ls2, ls3 on controller group -g\n"
            " -U <x.y.z> <ls3>\n"
            "           Unlinks -d as controller, x.y.z as responder on controller group -g and with ls3 on responder\n"
            " <ComPort> is required. On Windows COMn, on Linux, /dev/ttyUSBn\n\n"
            "Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.\n"
            "See license at http://github.com/w5xd/plm/blob/master/LICENSE.md\n"
            );
        
        return -1;
    }
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            if (strlen(modName))
            {
                fprintf(stderr, "Can only give one modem name but got both %s and %s\n", argv[i], modName);
                return -1;
            }
            modName = argv[i];
        }
        else switch (argv[i][1])
        {
            case 'r':
                reset = 1;
                break;
            case 'p':
                printModemLinks = 1;
                break;
            case 'g':
                if ((i < argc - 1) && isdigit(argv[i+1][0]))
                    linkGroup = atoi(argv[++i]);
                break;
            case 'l':
                cmdStartLink = 1;
                if (linkGroup < 0) linkGroup = 254; /* set default */
                break;
            case 'L':
                cmdStartLink = -1;
                if (linkGroup < 0) linkGroup = 254; /* set default */
                break;
            case 'd':
                if (i < argc-1)
                {
                    i += 1;
                    if (strcmp(argv[i],"pl") == 0)
                        printDimmerLinks = 1;
                    else
                        dimmerAddr = argv[i];
                }
                break;
            case 'e':
                printExtRecords = 1;
                if ((i < argc-1) && isdigit(argv[i+1][0]))
                {
                    printExtRecords = atoi(argv[++i]);
                    if (printExtRecords > 8) printExtRecords = 8;
                }
                break;
            case 's':
                if (i < argc-1)
                {
                    setVal = atoi(argv[++i]);
                    if (setVal == -1)
                        dimmerTest = 1;
                }
                break;
            case 'w':
                waitSeconds = 30;
                if ((i < argc - 1) && isdigit(argv[i+1][0]))
                    waitSeconds = atoi(argv[++i]);                
                break;
            case 'x':
                createLinks = 1;
                break;
            case 'X':
                if (strcmp(argv[i], "-X") == 0)
                    deleteLinks = 1;
                else if (strcmp(argv[i], "-X10") == 0)
                {
                    if ((i < argc - 2) && argv[i+1][0] != '-')
                    {
                        X10Hc = toupper(argv[++i][0]);
                        X10Unit = atoi(argv[++i]);
                    }
                    else
                        X10 = 1;
                }
                else
                {
                    fprintf(stderr, "command option %s invalid\n", argv[i]);
                    return 1;
                }
                break;
            case 'a':
                allStuff = 1;
                break;
            case 'R':
                if (i < argc - 1)
                {
                    responderLink = argv[++i];
                    if ((i < argc - 1) && isdigit(argv[i+1][0]))
                    {
                        ls1 = atoi(argv[++i]);  
                        if ((i < argc - 1) && isdigit(argv[i+1][0]))
                        {
                            ls2 = atoi(argv[++i]);
                            if ((i < argc - 1) && isdigit(argv[i+1][0]))
                                ls3 = atoi(argv[++i]);
                        }
                    }
                }
                if (!responderLink || (ls1 < 0) || (ls2 < 0) || (ls3 < 0))
                {
                    fprintf(stderr, "-R must specify <addr> <n1> <n2> <n3> where n1, n2 and n3 are the numbers for ls1 and ls2 and ls3\n");
                    return 1;
                }
                break;
            case 'U':
                if (i < argc - 1)
                {
                    responderUnlink = argv[++i];
                    if ((i < argc - 1) && isdigit(argv[i+1][0]))
                    {
                        ls3 = atoi(argv[++i]);  
                    }
                }
                if (!responderUnlink || (ls3 < 0))
                {
                    fprintf(stderr, "-U must specify addr n1 where n1 is the number for ls3\n");
                    return 1;
                }
                break;
            default:
                fprintf(stderr, "unrecognized command line argument %s\n", argv[i]);
                return -1;
        }
    }

    if (createLinks && !dimmerAddr)
    {
        fprintf(stderr, "Can't do -c unless also do -d\n");
        return -1;
    }

    if (printExtRecords && !dimmerAddr)
    {
        fprintf(stderr, "Can't do -e unless also do -d\n");
        return -1;
    }

    if (deleteLinks)
        if ((linkGroup < 0) || dimmerAddr || createLinks || setVal >= 0)
    {
        fprintf(stderr, "Can't remove without also only specifying -g <group>\n");
        return 1;
    }

    if ((cmdStartLink < 0) && (linkGroup < 0))
    {
        fprintf(stderr, "With -L, must also specify -g\n");
        return 1;
    }

    if ((responderLink || responderUnlink) && ((linkGroup < 0) || !dimmerAddr))
    {
        fprintf(stderr, "With -r or -R, must also specify -g and -d\n");
        return 1;
    }

    if (!modName || !modName[0])
    {
        fprintf(stderr, "Must specify the device name of the modem serial port\n");
        return 1;
    }

    if ((X10 || X10Hc) && !dimmerAddr)
    {
        fprintf(stderr, "With -X10 must specify -d\n");
        return 1;
    }

    if (X10 && !printExtRecords)
        printExtRecords = 1;
 
    m = openPowerLineModem(modName, 0, 2, 0);

    if (m == 0)
    {
        fprintf(stderr, "Failed to open modem %s\n", modName);
        return 1;
    }

    setErrorLevel(m, 2);

    if (reset)
    {
        clearModemLinkData(m);
        return 0;
    }

    getModemLinkRecords(m);

    if (printModemLinks)
        printModemLinkTable(m);

    if (allStuff)
    {
        Dimmer d1 = getDimmerAccess(m, "11.11.11");
        Dimmer d2 = getDimmerAccess(m, "22.22.22");
        Dimmer d3 = getDimmerAccess(m, "33.33.33");
        Dimmer d4 = getDimmerAccess(m, "44.44.44");
        setDimmerValue(d1, setVal);
        setDimmerValue(d2, setVal);
        setDimmerValue(d3, setVal);
        setDimmerValue(d4, setVal);
    }

    if (cmdStartLink > 0)
    {
        startLinking(m,linkGroup,1);
	    SLEEP(4*60);
        cancelLinking(m);
        return 0;
    }
    else if (cmdStartLink < 0)
    {
        if (dimmerAddr)
        {
            /* enter link mode AFTER the dimmer... */
        }
        else
        {
            startLinkingR(m, linkGroup);
	        SLEEP(4*60);
            cancelLinking(m);
            return 0;
       }
    }
    else
        cancelLinking(m);

    if (deleteLinks)
    {
        deleteGroupLinks(m, linkGroup);
        return 0;
    }

    if ((setVal >= 0) && (linkGroup >= 0))
        setAllDevices(m, linkGroup, (unsigned char)setVal);

    if (dimmerAddr)
    {
        int jj;
        dimmer = getDimmerAccess(m, dimmerAddr);
        if (!dimmer)
        {
            fprintf(stderr, "Invalid dimmer address %s\n", dimmerAddr);
            return 1;
        }

        if (printDimmerLinks)
        {
            int links;
            startGatherLinkTable(dimmer);
            links = getNumberOfLinks(dimmer);
            printLinkTable(dimmer);
        }
        for (jj = 1; jj <= printExtRecords; jj++)
        {
            int ret = extendedGet(dimmer, (unsigned char)jj, 0, 0);
        }
        for (jj = 1; jj <= printExtRecords; jj++)
             printExtendedGet(dimmer, (unsigned char)jj);

        if (X10)
        {
            char houseCode; unsigned char unit;
            int v = getX10Code(dimmer, &houseCode, &unit);
            if (v > 0)
            {
                if (houseCode)
                    fprintf(stderr, "getX10Code is %c and %d\n", houseCode, unit);
                else
                    fprintf(stderr, "getX10Code shows X10 code not set=n");
            }
            else
                fprintf(stderr, "getX10Code failed with error code %d\n", v);
        }

        if (X10Hc)
        {
            int v;
            if (X10Unit == 0)
                X10Hc = 0;
            v = setX10Code(dimmer, (char)X10Hc, (unsigned char)X10Unit);
            fprintf(stderr, "setX10Code completed %s\n", (v > 0 ? "OK" : "with error"));
        }

        if (createLinks)
        {
            if (linkGroup <= 0)
                linkGroup = getNextUnusedControlGroup(m);
            createModemGroupToMatch(linkGroup, dimmer);
            printf("Created link group for dimmer %s on group %d\n", dimmerAddr, linkGroup);
        }
        else if (cmdStartLink < 0)
        {
            linkAsController(dimmer, linkGroup);
            SLEEP(1);
        }
        else
        {
            int v;
            if (setVal < 0)
            {
                v = getDimmerValue(dimmer, 0);
                fprintf(stdout, "Getdimmer value=%d\n", v);
                if (dimmerTest)
                    DimmerTest(dimmer);
            }
            else
            {
                setDimmerValue(dimmer, setVal);
                SLEEP(1);
                v = getDimmerValue(dimmer, 1);
                fprintf(stdout, "Getdimmer value=%d\n", v);
            }
        }

        if (responderUnlink)
        {
            Dimmer responder = getDimmerAccess(m, responderUnlink);
            if (!responder)
            {
                fprintf(stderr, "Dimmer address %s is invalid\n", responderUnlink);
                return 1;
            }
            {
                int cLinks, v, rLinks;
                if (!printDimmerLinks)
                    startGatherLinkTable(dimmer);
                cLinks = getNumberOfLinks(dimmer);
                startGatherLinkTable(responder);
                rLinks = getNumberOfLinks(responder);
                if ((rLinks >= 0) && (cLinks >= 0))
                {
                    v = removeDeviceLink(dimmer, responder, linkGroup, ls3);
                    fprintf(stderr, "removeDeviceLink result is %d\n", v);
                }
                else
                    fprintf(stderr, "no unlinking attempted because rLinks=%d and cLinks=%d\n", rLinks, cLinks);
            }
        }
        else if (responderLink)
        {
            Dimmer responder = getDimmerAccess(m, responderLink);
            if (!responder)
            {
                fprintf(stderr, "Dimmer address %s is invalid\n", responderLink);
                return 1;
            }
            {
                int cLinks, v, rLinks;
                if (!printDimmerLinks)
                    startGatherLinkTable(dimmer);
                cLinks = getNumberOfLinks(dimmer);
                startGatherLinkTable(responder);
                rLinks = getNumberOfLinks(responder);
                if ((rLinks >= 0) && (cLinks >= 0))
                {
                    v = createDeviceLink(dimmer, responder, linkGroup, ls1, ls2, ls3);
                    startGatherLinkTable(dimmer);
                    getNumberOfLinks(dimmer);
                    startGatherLinkTable(responder);
                    getNumberOfLinks(responder);
                    printLinkTable(dimmer);
                    printLinkTable(responder);
                }
                else
                    fprintf(stderr, "no linking attempted because rLinks=%d and cLinks=%d\n", rLinks, cLinks);
            }
         }
    }

    SLEEP(waitSeconds);
    shutdownModem(m);

    return 0;
}

static void DimmerTest(Dimmer dimmer)
{
    setDimmerValue(dimmer, 0);
    SLEEP(1);
    setDimmerValue(dimmer, 128);
    SLEEP(1);
    setDimmerValue(dimmer, 255);
    SLEEP(1);
    setDimmerValue(dimmer, 128);
    SLEEP(1);
    setDimmerValue(dimmer, 0);
    SLEEP(1);
    setDimmerValue(dimmer, 255);
    SLEEP(1);
    setDimmerValue(dimmer, 0);
    SLEEP(1);
    setDimmerFast(dimmer, 1);
    SLEEP(1);
    setDimmerFast(dimmer, -1);
    SLEEP(1);
}
