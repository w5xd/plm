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
    bless $self, $class;
    return $self;
}

sub wasOpen {
    my $self = shift;
    return $self->{_wasOpen};
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
        return PowerLineModule::Dimmer->new($dimmer);
    }
    else { return 0; }
}

sub getKeypad {
    my $self = shift;
    my $keypad = PowerLineModule::getKeypadAccess( $self->{_modem}, shift );
    if ( $keypad != 0 ) {
        return PowerLineModule::Keypad->new($keypad);
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
