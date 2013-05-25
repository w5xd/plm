# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl PowerLineModule.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;

use Test::More tests => 7;
BEGIN { use_ok('PowerLineModule') };

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.
BEGIN {
	my $Msg = PowerLineModule::getErrorMessage(-1); 
	ok($Msg eq "CreateFile failed", "getErrorMsg");
	diag("Need two env variabls, POWERLINEMODULE_COMPORT and POWERLINEMODULE_DEVID\n");
	if (!defined($ENV{POWERLINEMODULE_COMPORT}) || !defined($ENV{POWERLINEMODULE_DEVID})) { return 1; }
	diag("They are now ".$ENV{POWERLINEMODULE_COMPORT}." and ".$ENV{POWERLINEMODULE_DEVID}."\n");
	my @Modem;
	@Modem = PowerLineModule::openPowerLineModem($ENV{POWERLINEMODULE_COMPORT}, 2, 0);
	ok(scalar(@Modem) == 2, "openPowerLineModem");
	diag("Modem: " . $Modem[0] . "," . $Modem[1] ."\n");
	if ($Modem[0] != 0) {
		my $Dimmer = PowerLineModule::getDimmerAccess($Modem[0], $ENV{POWERLINEMODULE_DEVID});
		ok($Dimmer != 0, "accessDimmer");
		diag("Dimmer: ".$Dimmer."\n");
		if ($Dimmer != 0) {
			my $v = PowerLineModule::getDimmerValue($Dimmer, 0);
			ok($v == 0, "setDimmerValue");
			PowerLineModule::setDimmerValue($Dimmer, 100);
			sleep(5);
			diag("Setting Dimmer to 0\n");
			PowerLineModule::setDimmerValue($Dimmer, 0);
			sleep(5);
			my @dX10 = PowerLineModule::getX10Code($Dimmer);
			ok(scalar(@dX10) == 3, "getX10Code");
			diag("getX10Code=".$dX10[0].", hc=".$dX10[1].", unit=".$dX10[2]."\n");
		}	
		my $Keypad = PowerLineModule::getKeypadAccess($Modem[0], $ENV{POWERLINEMODULE_DEVID});
		ok ($Keypad != 0, "accessKeypad");
		if ($Keypad != 0) {
			sleep(10);
			PowerLineModule::setWallLEDbrightness($Keypad, 32);
		}
	}
};
