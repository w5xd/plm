# Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
# See license at http://github.com/w5xd/plm/blob/master/LICENSE.md
package PowerLineModule::Modem;
# class that wraps the w5xdInsteon.dll modem object
#
use 5.010001;
use strict;
use warnings;
require PowerLineModule;
require PowerLineModule::Dimmer;
require PowerLineModule::Keypad;
require PowerLineModule::Fanlinc;

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
    $self->{_dimmerHash} = {};
    my $modem = delete $self->{_modem};
    return PowerLineModule::shutdownModem( $modem );
}

sub logFileName {
    my $self = shift;
    return $self->{_logfileName};
}

sub printLogString {
    my $self = shift;
    return PowerLineModule::printLogString( $self->{_modem}, shift );
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
	my $ret = $self->{_dimmerHash}->{$keypad};
	if (defined($ret)) { return $ret; }
        $ret = PowerLineModule::Keypad->new($keypad);
	$self->{_dimmerHash}->{$keypad} = $ret;
	return $ret;
    }
    else { return 0; }
}

sub getFanlinc {
    my $self = shift;
    my $fanlinc = PowerLineModule::getFanlincAccess( $self->{_modem}, shift );
    if ( $fanlinc != 0 ) {
	my $ret = $self->{_dimmerHash}->{$fanlinc};
	if (defined($ret)) { return $ret; }
        $ret = PowerLineModule::Fanlinc->new($fanlinc);
	$self->{_dimmerHash}->{$fanlinc} = $ret;
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

#NOTE: when called by threads->create the Modem object is COPIED, as are
#the Dimmer's and Keypad's that it references. So get___Access() functions
#all have to be called before thread->create(). 
sub monitor {
    my $self = shift;
    my $waitSecs = shift; #if < 0 waits forever and should be on own thread
    my $ret = 0;
    while (1) {
	    my @res = PowerLineModule::monitor($self->{_modem}, $waitSecs);
	    if (($#res < 7) || !$res[0]) { return $ret; }
	    # res[0] is non-zero to contiue
	    # res[1] is C pointer to InsteonDevice receiving notice
	    # res[2] is insteon group #
	    # res[3] is insteon cmd1
	    # res[4] is insteon cmd2
	    # res[5] is link ls1. Will be 0 unless getModemLinkRecords previously called
	    # res[6] is link ls2
	    # res[7] is link ls3
            $ret += 1;
	    my $dmhash = $self->{_dimmerHash};
	    my $dm = $dmhash->{$res[1]};
	    my $dmcb = $dm->monitorCb();
	    # invoke callbacks
	    shift @res; # remove the first array entry (useless to callbacks)
            if (defined($dm) && defined ($dmcb)) {
		    $res[0] = $dm; # replace C pointer with perl Dimmer reference
		    $dmcb->(@res); }
    }
}

# while setMonitorState is non-zero, the dll saves all notifications until
# called on its monitor method. Call setMonitorState(0) to let it dispose
# of notifications not yet delivered.
sub setMonitorState {
    my $self = shift;
    my $newState = shift;
    PowerLineModule::setMonitorState($self->{_modem}, $newState);
}

1;
