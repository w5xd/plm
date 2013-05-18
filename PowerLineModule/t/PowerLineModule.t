# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl PowerLineModule.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;

use Test::More tests => 5;
BEGIN { use_ok('PowerLineModule') };

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.
BEGIN {
	my $Msg = PowerLineModule::getErrorMessage(-1); 
	ok($Msg eq "CreateFile failed", "getErrorMsg");
	diag("Got error message " . $Msg . "\n");
	my @Modem;
	@Modem = PowerLineModule::openPowerLineModem("COM5", 0);
	ok(scalar(@Modem) == 2, "openPowerLineModem");
	diag("Modem: " . $Modem[0] . "," . $Modem[1] ."\n");
	if ($Modem[0] != 0) {
		my $Dimmer = PowerLineModule::getDimmerAccess($Modem[0], "1a.cd.8b");
		ok($Dimmer != 0, "accessDimmer");
		diag("Dimmer: ".$Dimmer."\n");
		if ($Dimmer != 0) {
			my $v = PowerLineModule::getDimmerValue($Dimmer);
			ok($v == -10, "setDimmerValue");
			PowerLineModule::setDimmerValue($Dimmer, 100);
			sleep(5);
			diag("Setting Dimmer 0\n");
			PowerLineModule::setDimmerValue($Dimmer, 0);
			sleep(5);
		}	
	}
};
