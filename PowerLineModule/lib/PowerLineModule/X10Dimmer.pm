# Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
# See license at http://github.com/w5xd/plm/blob/master/LICENSE.md
package PowerLineModule::X10Dimmer;
use 5.010001;
use strict;
use warnings;

require PowerLineModule;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = { _dimmer => shift };
    bless $self, $class;
    return $self;
}

sub destruct {
   my $self = shift;
   $self->{_dimmer} = 0;
}

sub setValue {
    my $self = shift;
    return PowerLineModule::setX10DimmerValue( $self->{_dimmer}, shift );
}

#without arg, returns name property. With an arg, sets it and returns it
sub name {
	my $self = shift;
	if (@_) {$self->{_name} = shift; }
	return $self->{_name};
}

1;
