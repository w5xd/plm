package PowerLineModule::Keypad;
use 5.010001;
use strict;
use warnings;
    require PowerLineModule;
    our @ISA = qw(PowerLineModule::Dimmer);    # inherits from Dimmer

    sub new {
        my ($class) = @_;

        # Call the constructor of the parent class, Dimmer.
        my $self = $class->SUPER::new( $_[1] );
        $self->{_keypad} = $_[1];
        bless $self, $class;
        return $self;
    }

    sub setWallLEDbrightness() {
        my $self = shift;
        return PowerLineModule::setWallLEDbrightness( $self->{_keypad}, shift );
    }

    sub extendedGet {
        my $self = shift;
        return PowerLineModule::extendedGet( $self->{_keypad}, shift );
    }

    sub printExtendedGet {    #nothing to print until after call to extendedGet
        my $self = shift;
        return PowerLineMOdule::printExtendedGet( $self->{_keypad}, shift );
    }
1;
