# Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
# See license at http://github.com/w5xd/plm/blob/master/LICENSE.md
package PowerLineModule::Modem;
use 5.010001;
use strict;
use warnings;
require PowerLineModule;
require PowerLineModule::Dimmer;
require PowerLineModule::Keypad;

sub new {
    my $class = shift;
    my $self  = {
        _commPort    => shift,
        _level       => shift,
        _logfileName => shift,
    };
    my @res = PowerLineModule::openPowerLineModem( $self->{_commPort},
        $self->{_level}, $self->{_logfileName} );
    $self->{_modem}   = $res[0];
    $self->{_wasOpen} = $res[1];
    $self->{_dimmerHash} = {};
    $self->{_keypadHash} = {};
    bless $self, $class;
    return $self;
}

sub wasOpen {
    my $self = shift;
    return $self->{_wasOpen};
}

sub shutdown {
    my $self = shift;
    my $dimmerHash = $self->{_dimmerHash};
    foreach my $dm (keys %$dimmerHash) { $dimmerHash->{$dm}->destruct();  } 
    my $kpHash = $self->{_keypadHash};
    foreach my $kp (keys %$kpHash) { $kpHash->{$kp}->destruct();    } 
    $self->{_dimmerHash} = {};
    $self->{_keypadHash} = {};
    my $modem = delete $self->{_modem};
    return PowerLineModule::shutdownModem( $modem );
}

sub logFileName {
    my $self = shift;
    return $self->{_logfileName};
}

sub commPort {
    my $self = shift;
    return $self->{_commPort};
}

sub openOk {
    my $self = shift;
    return $self->{_modem} != 0;
}

sub getDimmer {
    my $self = shift;
    my $dimmer = PowerLineModule::getDimmerAccess( $self->{_modem}, shift );
    if ( $dimmer != 0 ) {
	my $ret = $self->{_dimmerHash}->{$dimmer};
	if (defined($ret)) { return $ret; }
        $ret = PowerLineModule::Dimmer->new($dimmer);
	$self->{_dimmerHash}->{$dimmer} = $ret;
	return $ret;
    }
    else { return 0; }
}

sub getKeypad {
    my $self = shift;
    my $keypad = PowerLineModule::getKeypadAccess( $self->{_modem}, shift );
    if ( $keypad != 0 ) {
	my $ret = $self->{_keypadHash}->{$keypad};
	if (defined($ret)) { return $ret; }
        $ret = PowerLineModule::Keypad->new($keypad);
	$self->{_keypadHash}->{$keypad} = $ret;
	return $ret;
    }
    else { return 0; }
}

sub getModemLinkRecords {
    my $self = shift;
    return PowerLineModule::getModemLinkRecords( $self->{_modem} );
}

sub printLinkTable {    #nothing to print until after getModemLinkRecords
    my $self = shift;
    return PowerLineModule::printModemLinkTable( $self->{_modem} );
}

sub setAllDevices {
    my $self  = shift;
    my $group = shift;
    my $val   = shift;
    return PowerLineModule::setAllDevices( $self->{_modem}, $group, $val );
}

sub getNextUnusedControlGroup {
    my $self = shift;
    return PowerLineModule::getNextUnusedControlGroup( $self->{_modem} );
}

sub deleteGroupLinks {
    my $self  = shift;
    my $group = shift;
    return PowerLineModule::deleteGroupLinks( $self->{_modem}, $group );
}

sub cancelLinking {
   my $self = shift;
   return PowerLineModule::cancelLinking( $self->{_modem} );
}

#Two more args--dimmer callback function and keypad callback function
#NOTE: when called by threads->create the Modem object is COPIED, as are
#the Dimmer's and Keypad's that it references. So get___Access() functions
#all have to be called before thread->create(). 
sub monitor {
    my $self = shift;
    my $waitSecs = shift; #if < 0 waits forever and should be on own thread
    my $dimcb = shift;
    my $kpcb = shift;
    while (1) {
	    my @res = PowerLineModule::monitor($self->{_modem}, $waitSecs);
	    if (($#res < 7) || !$res[0]) { return; }
	    # res[0] is non-zero to contiue
	    # res[1] is C pointer to InsteonDevice receiving notice
	    # res[2] is insteon group #
	    # res[3] is insteon cmd1
	    # res[4] is insteon cmd2
	    # res[5] is link ls1. Will be 0 unless getModemLinkRecords previously called
	    # res[6] is link ls2
	    # res[7] is link ls3
	    my $kphash = $self->{_keypadHash};
	    my $dmhash = $self->{_dimmerHash};
	    my $kp = $kphash->{$res[1]};
	    my $dm = $dmhash->{$res[1]};
	    # invoke callbacks
	    shift @res; # remove the first array entry (useless to callbacks)
	    if (defined($kp) && defined($kpcb)) {   
		    $res[0] = $kp; # replace C pointer with perl Keypad reference
		    $kpcb->(@res); }
            elsif (defined($dm) && defined ($dimcb)) {
		    $res[0] = $dm; # replace C pointer with perl Dimmer reference
		    $dimcb->(@res); }
    }
}

sub setMonitorState {
    my $self = shift;
    my $newState = shift;
    PowerLineModule::setMonitorState($self->{_modem}, $newState);
}

1;
