# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl PowerLineModule.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;

use Test::More tests => 9;
use PowerLineModule::Modem;
use PowerLineModule::Dimmer;
use PowerLineModule::Keypad;
use threads;

BEGIN { use_ok('PowerLineModule') };

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.
BEGIN {

	sub dimmer_notify { 
		print STDERR "\ndimmer_notify:\n";
		foreach (@_) {	print STDERR " arg: ".$_; }
		print STDERR "\ndimmer_notify done\n";
	}

	sub kp_notify {
		print STDERR "\nkp_notify:\n";
		foreach (@_) {	print STDERR " arg: ".$_; }
		print STDERR "\nkp_notify done\n";
	}

	sub monitor_thread {
		my $mdm = shift;
		$mdm->monitor(-1, #wait forever
			\&dimmer_notify, #dimmer callback
			\&kp_notify	#keypad callback
		);
	}
        
	my $sleep = 0;
	if (defined($ENV{POWERLINEMODULE_SLEEP})) { 
			$sleep = $ENV{POWERLINEMODULE_SLEEP}; }
	if ($sleep > 0) {
		diag("sleeping ".$sleep."\n");
	       	sleep ($sleep); }
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
			my $d2 = $mdm->getDimmer($ENV{POWERLINEMODULE_DEVID});
			ok($d2 == $Dimmer, "two dimmers the same");
			$Dimmer->setValue(100);
			diag("sleeping 5"); sleep(5);
			my $v = $Dimmer->getValue(0);
			ok($v == 100, "getDimmerValue");
			diag("Setting Dimmer to 0\n");
			$Dimmer->setValue(0);
			diag("sleeping 5"); sleep(5);
			my $egRes = $Dimmer->extendedGet();
			ok($egRes eq 0, "PowerLineModule::Dimmer::extendedGet");
			diag($Dimmer->printExtendedGet());
			diag("X10HouseCode=".$Dimmer->x10House().", unit=".$Dimmer->x10Unit."\n");
			$Dimmer->startGatherLinkTable();
			my $nLinks = $Dimmer->getNumberOfLinks();
			my $lTable = $Dimmer->printLinkTable();
			if (defined $lTable) {diag("Dimmer has ".$nLinks." links.\n".$lTable);}
		}	
		my $Keypad = $mdm->getKeypad($ENV{POWERLINEMODULE_DEVID});
		my $kp2 = $mdm->getKeypad($ENV{POWERLINEMODULE_DEVID}); #repeat. should get same answer
		ok ($Keypad == $kp2, "get same twice");
		ok ($Keypad != 0, "accessKeypad");
		if ($Keypad != 0) {
			$Keypad->setValue(0);
			diag("sleeping 5"); sleep(5);
			$Keypad->setValue(1);
			diag("sleeping 5"); sleep(5);
			$Keypad->setWallLEDbrightness(32);
		}
		# once the getAbcAccess() are all called, then can create the monitor thread
		my $thr = threads->create('monitor_thread', $mdm); #COPIES $mdm hash
		if ($sleep != 0) { 
			if ($sleep < 0) { $sleep = -$sleep; }
			diag("sleeping ".$sleep."\n"); sleep($sleep); }
		$mdm->setMonitorState(0);
		#the following order is important. This main thread must command the shutdown
		#else the $thr thread will not exit and the join() will hang.
		$mdm->shutdown();
		$thr->join();
	}
	1;
};
