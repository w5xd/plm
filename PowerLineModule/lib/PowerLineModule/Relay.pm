# Copyright (c) 2016 by Wayne Wright, Round Rock, Texas.
# See license at http://github.com/w5xd/plm/blob/master/LICENSE.md
package PowerLineModule::Relay;
use 5.010001;
use strict;
use warnings;
require PowerLineModule;
require PowerLineModule::Dimmer;

our @ISA = qw(PowerLineModule::Dimmer);    # inherits from Dimmer

#A hardware relay can be driven by the Dimmer class and
#it "does the right thing"
#But if you have a hardware dimmer that you want to behave
#like a relay, then this is a way to do it. 

sub new {
    my ($class) = @_;
    # Call the constructor of the parent class, Dimmer.
    my $self = $class->SUPER::new( $_[1] );
    bless $self, $class;
    return $self;
}

sub setValue {
    my $self = shift;
    my $val = shift != 0 ? 255 : 0;
    return $self->SUPER::setValue($val);
}


1;
