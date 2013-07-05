# Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
# See license at http://github.com/w5xd/plm/blob/master/LICENSE.md
package PowerLineModule::Fanlinc;
use 5.010001;
use strict;
use warnings;
require PowerLineModule;
our @ISA = qw(PowerLineModule::Dimmer);    # inherits from Dimmer

sub new {
    my ($class) = @_;

    # Call the constructor of the parent class, Dimmer.
    my $self = $class->SUPER::new( $_[1] );
    $self->{_fanlinc} = $_[1];
    bless $self, $class;
    return $self;
}

sub setFanSpeed() {
    my $self = shift;
    return PowerLineModule::setFanSpeed( $self->{_fanlinc}, shift );
}

1;
