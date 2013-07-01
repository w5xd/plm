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

1;
