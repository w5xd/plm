# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl PowerLineModule.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;

use Test::More tests => 7;
use PowerLineModule::Modem;
use PowerLineModule::Dimmer;
use PowerLineModule::Keypad;

BEGIN { use_ok('PowerLineModule') };

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.
BEGIN {
	my $Msg = PowerLineModule::getErrorMessage(-1); 
	ok($Msg eq "CreateFile failed", "getErrorMsg");
	diag("Need two env variables, POWERLINEMODULE_COMPORT and POWERLINEMODULE_DEVID\n");
	if (!defined($ENV{POWERLINEMODULE_COMPORT}) || !defined($ENV{POWERLINEMODULE_DEVID})) { return 1; }
	diag("They are read as: ".$ENV{POWERLINEMODULE_COMPORT}." and ".$ENV{POWERLINEMODULE_DEVID}."\n");

	my $mdm = PowerLineModule::Modem->new($ENV{POWERLINEMODULE_COMPORT}, 
		0, #no diagnostics to log file
		"" #no log file to write to
	);
	my $res = $mdm->openOk();
        ok($res eq 1, "Modem openOk");
	if ($res) {
		my $mlr = $mdm->getModemLinkRecords();
		diag("numModemLinks=".$mlr."\n".$mdm->printLinkTable());
		my $Dimmer = $mdm->getDimmer($ENV{POWERLINEMODULE_DEVID});
		ok($Dimmer != 0, "accessDimmer");
		if ($Dimmer != 0) {
			$Dimmer->setValue(100);
			sleep(5);
			my $v = $Dimmer->getValue(0);
			ok($v == 100, "getDimmerValue");
			diag("Setting Dimmer to 0\n");
			$Dimmer->setValue(0);
			sleep(5);
			my $egRes = $Dimmer->extendedGet();
			ok($egRes eq 0, "PowerLineModule::Dimmer::extendedGet");
			diag($Dimmer->printExtendedGet());
			diag("X10HouseCode=".$Dimmer->x10House().", unit=".$Dimmer->x10Unit."\n");
			$Dimmer->startGatherLinkTable();
			my $nLinks = $Dimmer->getNumberOfLinks();
			my $lTable = $Dimmer->printLinkTable();
			diag("Dimmer has ".$nLinks." links.\n".$lTable);
		}	
		my $Keypad = $mdm->getKeypad($ENV{POWERLINEMODULE_DEVID});
		ok ($Keypad != 0, "accessKeypad");
		if ($Keypad != 0) {
			diag("Keypad dimmer ".$Keypad->{_dimmer}. " kp=".$Keypad->{_keypad}."\n");
			$Keypad->setValue(0);
			sleep(5);
			$Keypad->setValue(1);
			sleep(5);
			$Keypad->setWallLEDbrightness(32);
		}
	}
	1;
};
