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

static int procCommmand(Modem *m, int *readStdin, int *waitSeconds, int argc, char **argv);

int main (int argc, char **argv)
{
    int waitSeconds = 0;
    Modem m = 0;
    int readStdin = 0;
    int ret = procCommmand(&m, &readStdin, &waitSeconds, argc, argv);
    while ((ret == 0) && readStdin)
    {
        fputs("PlmTest> ", stdout);
        char buf[256];
        char *p = fgets(buf, sizeof(buf), stdin);
        if (!p || !*p)
            break;
        argc = 1;
        argv = malloc(argc * sizeof(char*));
        argv[0] = "";
        for (;;)
        {
            while (*p && isspace(*p)) *p++ = 0;
            if (!*p)
                break;
            argv = realloc(argv, (argc+1) * sizeof(char *));
            argv[argc++] = p;
            while (*p && !isspace(*p)) p += 1;
        }
        waitSeconds = 0;
        if (argc > 1)
            ret = procCommmand(&m, &readStdin, &waitSeconds, argc, argv);
        free(argv);
    }
    if (!waitSeconds)
        SLEEP(1);
    shutdownModem(m);
    return ret;
}

static int procCommmand (Modem *mp, int *readStdin, int *waitSeconds, int argc, char **argv)
{
    Modem m = 0;
    const char *modName = "";
    const char *dimmerAddr = 0;
    Dimmer dimmer=0;
    Fanlinc fanlinc=0;
    int flFlag = 0;
    int reset = 0;
    int cmdStartLink = 0; /* >0 is link as control, <0 is responder*/
    int cmdCancelLink = 0;
    int linkGroup = -1;
    int i;
    int setVal = -1;
    int createLinks = 0;
    int deleteLinks = 0;
    int dimmerTest = 0;
    int allStuff = 0;
    int printModemLinks = 0;
    int printDimmerLinks = 0;
    int printEngineVersion = 0;
    int suppressDimmerValuePrint = 0;
    int suppressLinkTableUpdateRes = 0;
    int suppressLinkTableUpdateCtrl = 0;
    int simulateSETbutton = 0;
    int simulateSETbuttonUnlink = 0;
    int printExtRecords = 0;
    int messageLevel = -1;
    int keypadButton = -1;
    int keypadMask = -1;
    char keypadArg = 0;
    const char *controllerLink = 0;
    const char *controllerUnlink = 0;
	int plmUnlinkAsController = 0;
    Dimmer controllerU = 0;
    int cLinks = -1;
    int X10Unit = -1;
    char X10Hc = 0;
    int X10 = 0;
	int X10Command = -1;
	short X10UnitMask = 0;
    int ls1 = -1, ls2 = -1, ls3 = -1;
	int groupCommand = -1;
    if (argc < 2)
    {
        fprintf(stderr, "Usage: PlmTest [-r] [-l] [-g grp] [-d x.y.z [-d pl]] [-s value] [-w [tmo]] [-x] [-X] <ComPort>\n");
        fprintf(stderr,
            " -Reset   Reset modem to factory defaults. All other commands ignored\n"
            " -r       Start linking process  as responder on group grp.\n"
            "          If -d not specified, then stay in link mode for 4 minutes and exit\n"
            "          If -d is specified, then set link tables so PLM responds to dimmer on group grp\n"
            " -l [stop]\n"
            "          Start linking process as controller on group grp (default to 254)\n"
            "          if -d is also specified, then DON'T start link process, but instead set\n"
            "          PLM as controller to -d x.y.z, on group -g (or next available PLM group if no -g)\n"
            " -L <x.y.z> [slu]\n"
            "          Links -d as responder, x.y.z as controller on controller group -g. 'slu', if present, supresses link table update\n"
            " -ls  <ls1> <ls2> <ls3>\n"
            "          When used with -l or -L specifies the ls1,ls2,ls3 entries (decimal) for the responder.\n"
            " -U <x.y.z> <ls3> [slu]\n"
            "           Unlinks -d as responder, x.y.z as controller on controller group -g and with ls3 on responder\n"
            "           If <x.y.z> not specified, then unlinks PLM as controller.\n"
            "           If -d not specified, then it unlinks PLM as responder.\n"
            "           If slu is specified, then device x.y.z's link is not updated.\n"
            " -p       Print modem link table\n"
            " -g <grp> Set group number for -s and -l to grp\n"
            " -d x.y.z Dimmer operations on Insteon address x,y.z\n"
            "          \n"
            " -fl      Fanlinc instead of dimmer. -s operates on fan\n"
            " -d pl    Print device link table. Must be combined with another -d\n"
            " -d ver   Print Insteon Engine version (currently 0 or 1). combine with -d\n"
            " -d npv   Do not query dimmer value. Must be combined with another -d\n"
            " -d slu   Supress link table update (so -L/-D work without fetching links)\n"
            " -d set   Simulate SET button hold. combine with -d\n"
            " -d setU  Simulate SET button hold twice for unlink. combine with -d\n"
            " -s <value> Command group grp or dimmer to on or off\n"
            "          -1 with -d runs a dimmer test sequence\n"
            "          Any other negative value just retrieves dimmer value\n"
			" -c <val> \n"
			"          Send command <val> (normally 17 for ON and 19 for off to group -g\n"
            " -kf <btn> <val>   keypad follow mask\n"
            " -ko <btn> <val>   keypad off mask\n"
            " -kX10 <btn> <hc> <unit>\n"
            "           for keypad at address -d, and for button number <btn>, set follow mask (or off mask, or X10 house code and unit)\n"
            " -w <tmo> Wait for tmo seconds (default to 30) and print monitored traffic\n"
            " -x       For the dimmer -d, cross link records on this modem to duplicate\n"
            "          its group 1 behavior when turned on/off from this modem\n"
            "          Specify a group number, -g, otherwise will use next unused group number in the PLM\n"
            " -X       for the group -g and PLM as controller, track down all links on this modem and on devices and delete them\n"
            "          adding -d x.y.z -d pl on the same command will delete those links even if modem is missing them\n"
            " -X10 [hc unit] for the dimmer -d,\n"
            "          print its X10 house code if hc unit not specified. Otherwise set X10 house code and unit.\n"
            "          A zero unit clears the X10 code in the device\n"
			" -x10 hc mask command\n"
			"          Send X10 hc ranges from A through P\n"
			"          mask is HEX. 0 is no units. and LSB is unit 1 MSB is unit 16\n"
            "          command: \n"
			"              0 = all units off\n"
			"              1 = all lights on\n"
			"              2 = unit ON\n"
			"              3 = unit OFF\n"
			"              4 = unit DIM\n"
			"              5 = unit BRIGHT\n"
			"              6 = all lights off\n"
            " -e [n]   For specified dimmer, print n extended records.\n"
            " -m <n>    Set message level to n.\n"
            " --        means read stdin for more commands\n"
            " <ComPort> is required. On Windows COMn, on Linux, /dev/ttyUSBn\n"
			" #         on command line is not processed, nor anything after it.\n"
            "\n"
            "Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.\n"
            "See license at http://github.com/w5xd/plm/blob/master/LICENSE.md\n"
            );
        
        return -1;
    }
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
			if (argv[i][0] == '#')
				break;
            if (strlen(modName))
            {
                fprintf(stderr, "Can only give one modem name but got both %s and %s\n", argv[i], modName);
                return -1;
            }
            modName = argv[i];
        }
        else switch (argv[i][1])
        {
            case 'R':
                if (strcmp(argv[i], "-Reset") == 0)
                    reset = 1;
                else
                {
                    fprintf(stderr, "Must spell out -Reset to reset the modem\n");
                    return -1;
                }
                break;
            case 'p':
                printModemLinks = 1;
                break;
            case 'm':
                if (i < argc -1)
                    messageLevel = atoi(argv[++i]);
                break;
            case 'g':
                if ((i < argc - 1) && isdigit(argv[i+1][0]))
                    linkGroup = atoi(argv[++i]);
                break;
            case 'l':
				if (!argv[i][2])
				{
                    if ((i < argc - 1) && 0 == strcmp("stop", argv[i + 1]))
                    {
                        cmdCancelLink = 1;
                        cmdStartLink = 0;
                        i += 1;
                    }
                    else
                    {
                        cmdStartLink = 1;
                        if (linkGroup < 0) linkGroup = 254; /* set default */
                    }
					break;
				}
				else if (argv[i][2] == 's')
				{
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
					if ((ls1 < 0) || (ls2 < 0) || (ls3 < 0))
					{
						fprintf(stderr, "-ls must specify <n1> <n2> <n3> where n1, n2 and n3 are the numbers for ls1 and ls2 and ls3\n");
						return 1;
					}
					break;
				}
				else
				{
                    fprintf(stderr, "Unknown switch \"%s\"\n", argv[i]);
                    return -1;
				}
            case 'L':
                if (i < argc - 1)
                    controllerLink = argv[++i];
                if (!controllerLink)
                {
                    fprintf(stderr, "-L must specify <link addr>\n");
                    return 1;
                }
                else if (i <= argc - 1 && strcmp(argv[i+1], "slu") == 0)
                {
                    suppressLinkTableUpdateCtrl = 1;
                    i += 1;
                }
                break;
           case 'r':
                cmdStartLink = -1;
                if (linkGroup < 0) linkGroup = 254; /* set default */
                break;
            case 'd':
                if (i < argc-1)
                {
                    i += 1;
                    if (strcmp(argv[i], "pl") == 0)
                        printDimmerLinks = 1;
                    else if (strcmp(argv[i], "ver") == 0)
                        printEngineVersion = 1;
                    else if (strcmp(argv[i], "npv") == 0)
                        suppressDimmerValuePrint = 1;
                    else if (strcmp(argv[i], "slu") == 0)
                        suppressLinkTableUpdateRes = 1;
                    else if (strcmp(argv[i], "set") == 0)
                    {
                        simulateSETbutton = 1;
                        simulateSETbuttonUnlink = 0;
                        suppressDimmerValuePrint = 1;
                    }
                    else if (strcmp(argv[i], "setU") == 0)
                    {
                        simulateSETbutton = 0;
                        simulateSETbuttonUnlink = 1;
                        suppressDimmerValuePrint = 1;
                    }
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
                if ((i < argc - 1) && isdigit(argv[i+1][0]))
                    *waitSeconds = atoi(argv[++i]);                
                break;
            case 'x':
				if (strcmp(argv[i], "-x") == 0)
				{
					createLinks = 1;
					break;
				}
				else if (strcmp(argv[i], "-x10") == 0)
				{
					X10UnitMask = 0;
					X10Command = -1;
					if ((i < argc - 1) && argv[i+1][0] != '-')
					{
                        X10Hc = toupper(argv[++i][0]);
						if ((i < argc - 1) && argv[i+1][0] != '-')
						{
							const char *pHex = argv[++i];
							if (strncmp(pHex, "0x", 2) == 0 ||
								strncmp(pHex, "0X", 2) == 0)
								pHex += 2;
							while (*pHex)
							{
								char next = *pHex++;
								if (isalpha(next))
									next = toupper(next);
								X10UnitMask <<= 4;
								if ((next >= '0') && (next <= '9'))
									X10UnitMask += next - '0';
								else if ((next >= 'A') && (next <= 'F'))
									X10UnitMask += 10 + next - 'A';
								else break;
							}
							if (!*pHex && i < argc - 1 && argv[i+1][0] != '-')
							{
								X10Command = atoi(argv[++i]) % 7;
								break;
							}
						}
					}
				}
                fprintf(stderr, "command option %s invalid\n", argv[i]);
                return 1;
            case 'X':
                if (strcmp(argv[i], "-X") == 0)
                    deleteLinks = 1;
                else if (strcmp(argv[i], "-X10") == 0)
                {
                    if ((i < argc - 2) && argv[i+1][0] != '-')
                    {
                        X10Hc = toupper(argv[++i][0]);
                        X10UnitMask = atoi(argv[++i]);
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
            case 'U':
                if (i < argc - 1)
                {
                    controllerUnlink = argv[++i];
					if (strchr(controllerUnlink, '.') != 0)
					{
						plmUnlinkAsController = 0;
                        if ((i < argc - 1) && isdigit(argv[i + 1][0]))
                        {
                            ls3 = atoi(argv[++i]);
                            if ((i < argc - 1) && strcmp(argv[i+1], "slu") == 0)
                            {
                                suppressLinkTableUpdateCtrl = 1;
                                i += 1;
                            }
                        }
					}
					else if (isdigit(controllerUnlink[0]))
					{
						ls3 = atoi(controllerUnlink);
						controllerUnlink = 0;
						plmUnlinkAsController = 1;
					}
                }
                if ((!controllerUnlink && !plmUnlinkAsController) || (ls3 < 0))
                {
                    fprintf(stderr, "-U must specify [addr] <n1> where n1 is the number for ls3\n");
                    return 1;
                }
                break;
            case '-':
                *readStdin = 1;
                break;
            case 'f':
                if (strcmp("-fl", argv[i]) == 0)
                    flFlag = 1;
                else
                {
                    fprintf(stderr, "unknown option %s\n", argv[i]);
                    return 1;
                }
                break;
            case 'k':
                if (i < argc - 2)
                {
                    keypadArg = 0;
                    if (keypadButton >= 0)
                    {
                        fprintf(stderr, "can only do one of -kf -ko -kX10\n");
                        return 1;
                    }
                    keypadButton = atoi(argv[i+1]);
                    keypadMask = atoi(argv[i + 2]);
                    if (strcmp(argv[i], "-kX10") != 0)
                     {
                         if (strcmp(argv[i], "-kf") == 0)
                             keypadArg = 'f';
                         else if (strcmp(argv[i], "-ko") == 0)
                             keypadArg = 'o';
                         if (keypadArg != 0)
                         {
                             i += 2;
                             break;
                         }
                     }
                     else if (i < argc - 3)
                     {
                        X10Hc = keypadMask;
                        keypadArg = 'x';
                        X10UnitMask = atoi(argv[i + 3]);
                        i += 3;
                        break;
                     }
                }
                fprintf(stderr, "unregcognized comand argument %s\n", argv[i]);
                return -1;
			case 'c':
				if ((i < argc - 1) && isdigit(argv[i+1][0]))
				{
					groupCommand = atoi(argv[++i]);
					break;
				}
				else
				{
					fprintf(stderr, "-c must be followed by <val> 17 for ON, 19 for OFF\n");
					return 1;
				}
            default:
                fprintf(stderr, "unrecognized command line argument %s\n", argv[i]);
                return -1;
        }
    }

    if (createLinks && !dimmerAddr)
    {
        fprintf(stderr, "Can't do -x unless also do -d\n");
        return -1;
    }

    if (printExtRecords && !dimmerAddr)
    {
        fprintf(stderr, "Can't do -e unless also do -d\n");
        return -1;
    }

    if (deleteLinks)
        if ((linkGroup < 0) || createLinks || setVal >= 0)
        {
            fprintf(stderr, "Can't remove without also only specifying -g <group>\n");
            return 1;
        }

    if ((cmdStartLink < 0) && (linkGroup < 0))
    {
        fprintf(stderr, "With -r, must also specify -g\n");
        return 1;
    }

    if (controllerLink  && ((linkGroup < 0) || !dimmerAddr))
    {
        fprintf(stderr, "With -r or -L, must also specify -g and -d\n");
        return 1;
    }

	if ((controllerUnlink || plmUnlinkAsController) && (!dimmerAddr || (linkGroup < 0)))
    {
        fprintf(stderr, "With -U, must also specify -g and -d \n");
        return 1;
    }

    if (keypadArg && !dimmerAddr)
    {
        fprintf(stderr, "With -k_ must also specify -d\n");
        return 1;
    }

    if (!*mp && (!modName || !modName[0]))
    {
        if ((argc == 2) && (strcmp(argv[1], "--") == 0))
            return 0;   // silently return to tell caller to read stdin
        fprintf(stderr, "Must specify the device name of the modem serial port\n");
        return 1;
    }

    if (linkGroup < 0 && (simulateSETbutton || simulateSETbuttonUnlink))
    {
        fprintf(stderr, "SET button operations require -g\n");
        return 1;
    }

    if (setVal >= 0 && simulateSETbuttonUnlink)
    {
        fprintf(stderr, "setU button operation requires there be no -s\n");
        return 1;
    }

    if ((X10 || (X10Hc && (X10Command < 0))) && !dimmerAddr)
    {
        fprintf(stderr, "With -X10 must specify -d\n");
        return 1;
    }

	if ((X10Command >= 0) && dimmerAddr)
	{
		fprintf(stderr, "With -x10 must not specifiy -d\n");
		return 1;
	}

    if (X10 && !printExtRecords)
        printExtRecords = 1;
 
    if (modName && *modName)
    {
        *mp = openPowerLineModem(modName, 0, 3, 0);
        if (*mp)
            setErrorLevel(*mp, 3);
    }
    m = *mp;

    if (m == 0)
    {
        fprintf(stderr, "Failed to open modem %s\n", modName);
        return 1;
    }

    if (messageLevel >= 0)
        setErrorLevel(m, messageLevel);

    if (reset)
    {
        clearModemLinkData(m);
        return 0;
    } 

    if (printModemLinks)
    {
        const char *modLinks;
        getModemLinkRecords(m);
        modLinks = 
            printModemLinkTable(m);
        if (modLinks)
            fprintf(stderr, "%s", modLinks);
    }

    if (dimmerAddr)
        dimmer = getDimmerAccess(m, dimmerAddr);

    if (flFlag)
    {
        fanlinc = getFanlincAccess(m, dimmerAddr);
        dimmer = (Dimmer)fanlinc;
    }

    if (cmdStartLink > 0)
    {
        if (!dimmerAddr)
        {
            cancelLinking(m);
            startLinking(m, linkGroup, 1);
            return 0;
        }
        else
		{
            if (linkGroup == 254 || linkGroup <= 0)
			{
				getModemLinkRecords(m);
                linkGroup = getNextUnusedControlGroup(m);
			}
			{
				int num = 0;
				startGatherLinkTable(dimmer);
				num = getNumberOfLinks(dimmer);
				if (num < 0)
				{
					fprintf(stderr, "Failed to get number of links for dimmer: %s\n", dimmerAddr);
					return 1;
				}
			}
			linkPlm(dimmer, 0, linkGroup, 
				ls1 > 0 ? ls1 : 0, 
				ls2 > 0 ? ls2 : 0, 
				ls3 > 0 ? ls3 : 2);
		}
    }
    else if (cmdStartLink < 0)
    {
        if (dimmerAddr)
        {
            /* enter link mode AFTER the dimmer... */
        }
        else
        {
            cancelLinking(m);
            startLinkingR(m, linkGroup);
            return 0;
       }
    }

    if ((setVal >= 0) && (linkGroup >= 0))
        setAllDevices(m, linkGroup, (unsigned char)setVal);

    if (controllerUnlink)
    {
        controllerU = getDimmerAccess(m, controllerUnlink);
        if (!controllerU)
        {
            fprintf(stderr, "Dimmer address %s is invalid\n", controllerUnlink);
            return 1;
        }
        if (!suppressLinkTableUpdateCtrl)
            startGatherLinkTable(controllerU);
        else
            suppressLinkTableUpdate(controllerU);
        cLinks = getNumberOfLinks(controllerU);    
        if (cLinks < 0)
            fprintf(stderr, "Can't get %s links, but continuing\n", controllerUnlink);
    }

    if (dimmerAddr)
    {
        int jj;
        Keypad kp = 0;
        if (keypadArg)
            kp = getKeypadAccess(m, dimmerAddr); // more derived class we ask for first

        if (!dimmer)
        {
            fprintf(stderr, "Invalid dimmer address %s\n", dimmerAddr);
            return 1;
        }

        if (printDimmerLinks)
        {
            int links;
            const char *dimLinks;
            startGatherLinkTable(dimmer);
            links = getNumberOfLinks(dimmer);
            dimLinks = printLinkTable(dimmer);
            if (dimLinks)
                fprintf(stderr, "%s", dimLinks);
        }
        if (printEngineVersion)
        {
            unsigned char v = getInsteonEngineVersion(dimmer);
            fprintf(stderr, "Insteon Engine Version: %d\n", (int)v);
        }
        if (simulateSETbutton)
            pressSetButtonLink(dimmer, linkGroup);
        else if (simulateSETbuttonUnlink)
            pressSetButtonUnlink(dimmer, linkGroup);
        for (jj = 1; jj <= printExtRecords; jj++)
        {
            int ret = extendedGet(dimmer, (unsigned char)jj, 0, 0);
            if (ret < 0)
            {
                fprintf(stderr, "extendedGet failed on item %d\n", (int)jj);
                break;
            }
        }
        for (jj = 1; jj <= printExtRecords; jj++)
        {
             const char * eg = printExtendedGet(dimmer, (unsigned char)jj);
             if (eg)
                 fprintf(stderr, "%s", eg);
        }

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
			{
				getModemLinkRecords(m);
                linkGroup = getNextUnusedControlGroup(m);
			}
            createModemGroupToMatch(linkGroup, dimmer);
            printf("Created link group for dimmer %s on group %d\n", dimmerAddr, linkGroup);
        }
        else if (cmdStartLink < 0)
        {
            linkPlm(dimmer, 1, (unsigned char)linkGroup, 0, 0, 2);
            SLEEP(1);
        }
		else if (plmUnlinkAsController)
		{
			unLinkPlm(dimmer, 0, (unsigned char)linkGroup, ls3);
		}
        else
        {
            if (setVal < 0)
            {
                int v;
                if (!suppressDimmerValuePrint)
                {
                    v = fanlinc ? getFanSpeed(fanlinc) : getDimmerValue(dimmer, 0);
                    fprintf(stdout, "Getdimmer value=%d\n", v);
                }
                if (dimmerTest)
                    DimmerTest(dimmer);
            }
            else
            {
                if (fanlinc)
                    setFanSpeed(fanlinc, setVal);
                else
                    setDimmerValue(dimmer, setVal);
            }
        }

        if (controllerU)
        {
            int v, rLinks;
            if (dimmer)
            {
                if (suppressLinkTableUpdateRes)
                    suppressLinkTableUpdate(dimmer);
                else {
                    if (isLinkTableComplete(dimmer) <= 0)
                        startGatherLinkTable(dimmer);
                }
                rLinks = getNumberOfLinks(dimmer);
                if ((rLinks < 0) && (cLinks < 0))
                    fprintf(stderr, "no unlinking attempted because rLinks=%d and cLinks=%d\n", rLinks, cLinks);
                else
                {
                    v = removeDeviceLink(controllerU, dimmer, linkGroup, ls3);
                    fprintf(stderr, "removeDeviceLink result is %d\n", v);
                }
            }
        }
        else if (controllerLink)
        {
            Dimmer controller = getDimmerAccess(m, controllerLink);
            if (!controller)
            {
                fprintf(stderr, "Dimmer address %s is invalid\n", controllerLink);
                return 1;
            }
			if (ls3 < 0)
			{
				fprintf(stderr, "With -L must also specify -ls\n");
				return 1;
			}
            {
                int cLinks, v, rLinks;
                if (suppressLinkTableUpdateRes)
                    suppressLinkTableUpdate(dimmer);
                else {
                    if (isLinkTableComplete(dimmer) <= 0)
                        startGatherLinkTable(dimmer);
                }
                rLinks = getNumberOfLinks(dimmer);
                if (suppressLinkTableUpdateCtrl)
                    suppressLinkTableUpdate(controller);
                else {
                    if (isLinkTableComplete(controller) <= 0)
                        startGatherLinkTable(controller);
                }
                cLinks = getNumberOfLinks(controller);
                if ((rLinks >= 0) && (cLinks >= 0))
                    v = createDeviceLink(controller, dimmer, linkGroup, ls1, ls2, ls3);
                else
                    fprintf(stderr, "no linking attempted because rLinks=%d and cLinks=%d\n", rLinks, cLinks);
            }
         }

        {
            int v = -100;
            switch (keypadArg)
            {
            case 'f':
                v = setKeypadFollowMask(kp, keypadButton, keypadMask);
                break;
            case 'o':
                v = setKeypadOffMask(kp, keypadButton, keypadMask);
                break;
            case 'x':
                v = setBtnX10Code(kp, (unsigned char)X10Hc, (unsigned char)X10UnitMask, keypadButton);
                break;
            }
            if (v != -100)
                fprintf(stderr, "Keypad mask setting result is %d\n", v);
        }
    }
    else if (controllerU)
    {
        int v = unLinkPlm(controllerU, 1, (unsigned char) linkGroup, 0);
        fprintf(stderr, "unLinkAsControll completed %d\n", v);
    }
	else if (X10Command >= 0)
			sendX10Command(m, X10Hc, X10UnitMask, (enum X10Commands_t)(X10Command));

    if (deleteLinks)
    {   // order of this command is important.
        // if -d appears for a dimmer, it will be checked for responding to this modem
        // even if the modem is missing a command link for it.
        int v = deleteGroupLinks(m, linkGroup);
        fprintf(stderr, "-X command returned %d\n", v);
    }

 	if (groupCommand > 0)
		setAllDevices(m, linkGroup, groupCommand);

    if (cmdCancelLink > 0)
        cancelLinking(m);

#if 1
    SLEEP(*waitSeconds);
#else   
    // The sensor components turn their receivers ON only when they want to transmit
    // This code sends them some query commands on receipt of a transmission from a sensor
    {
        int sleep = *waitSeconds;
        int started = 0;
        int extended = 0;
        if (sleep > 0)
            setMonitorState(m, 1);
        while (sleep-- > 0)
        {
            Dimmer d=0; unsigned char group=0, cmd1=0, cmd2=0, ls1, ls2, ls3;
            int ret = monitorModem(m, 1, &d, &group, &cmd1, &cmd2, &ls1, &ls2, &ls3);
            fprintf(stdout, "monitor ret:%d, %d, %d, %d\n", ret, (int)d, (int)group, (int) cmd1, (int) cmd2);
            if (!started && ret > 0)
            {
                started = 1;
                startGatherLinkTable(dimmer);
            }
            else if (started)
            {
                int links = getNumberOfLinks(dimmer);
                fprintf(stdout, "number of links: %d\n", links);
                if (links > 0)
                {
                    const char *c = printLinkTable(dimmer);
                    if (c)
                        fprintf(stdout, "%s", c);
                }
                if (!extended)
                {
                    extended = 1;
                    extendedGet(dimmer, 1, 0, 0);
                }
            }
                
        }
    }
#endif

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
