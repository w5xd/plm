# Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
# See license at http://github.com/w5xd/plm/blob/master/LICENSE.md
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
    my $btn = shift;
    return PowerLineModule::extendedGet( $self->{_keypad}, $btn );
}

sub printExtendedGet {    #nothing to print until after call to extendedGet
    my $self = shift;
    my $btn = shift;
    return PowerLineModule::printExtendedGet( $self->{_keypad}, $btn );
}

sub _getX10Code {
    my $self = shift;
    my $btn = shift;
    my @ret = PowerLineModule::getBtnX10Code($self->{_keypad}, $btn);
    my $hc = $self->{_kpX10House};
    if (!defined($hc)) { $hc = {};}
    my $un = $self->{_kpX10Unit};
    if (!defined($un)) { $un = {};}
    $hc->{$btn} = $ret[1];
    $un->{$btn} = $ret[2];
    $self->{_kpX10House} = $hc;
    $self->{_kpX10Unit} = $un;
}

sub x10House {
    my $self = shift;
    my $btn = shift;
    $self->_getX10Code($btn);
    return $self->{_kpX10House}{$btn};
}

sub x10Unit {
    my $self = shift;
    my $btn = shift;
    $self->_getX10Code($btn);
    return $self->{_kpX10Unit}{$btn};
}

sub setX10Code {
    my $self = shift;
    my $hc = shift;
    my $unit = shift;
    my $btn = shift;
    return PowerLineModule::setBtnX10Code($self->{_keypad}, $hc, $unit, $btn);
}

sub setBtnRampRate {
    my $self = shift;
    my $rate = shift;
    my $button = shift;
    return PowerLineModule::setBtnRampRate($self->{_keypad}, $rate, $button);
}

sub setBtnOnLevel {
    my $self = shift;
    my $level = shift;
    my $button =shift;
    return PowerLineModule::setBtnOnLevel($self->{_keypad}, $level, $button );
}

1;
