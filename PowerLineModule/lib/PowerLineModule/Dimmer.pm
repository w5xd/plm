# Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
# See license at http://github.com/w5xd/plm/blob/master/LICENSE.md
package PowerLineModule::Dimmer;
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

sub setValue {
    my $self = shift;
    return PowerLineModule::setDimmerValue( $self->{_dimmer}, shift );
}

sub setFast {
    my $self = shift;
    return PowerLineModule::setDimmerFast( $self->{_dimmer}, shift );
}

sub getValue {
    my $self = shift;
    return PowerLineModule::getDimmerValue( $self->{_dimmer}, shift );
}

sub _readX10Code {    #need to have previously called extendedGet
    my $self = shift;
    delete $self->{_x10House};
    delete $self->{_x10Unit};
    my @dX10 = PowerLineModule::getX10Code( $self->{_dimmer} );
    if ( $dX10[0] >= 0 ) {
        $self->{_x10House} = $dX10[1];
        $self->{_x10Unit}  = $dX10[2];
    }
    return $dX10[0];
}

sub x10House {        #need to have previously called extendedGet
    my $self = shift;
    $self->_readX10Code();
    return $self->{_x10House};
}

sub x10Unit {         #need to have previously called extendedGet
    my $self = shift;
    $self->_readX10Code();
    return $self->{_x10Unit};
}

sub setX10Code {
    my $self = shift;
    my $hc   = shift;
    my $unit = shift;
    return PowerLineModule::setX10Code( $self->{_dimmer}, $hc, $unit );
}

sub startGatherLinkTable {
    my $self = shift;
    return PowerLineModule::startGatherLinkTable( $self->{_dimmer} );
}

sub getNumberOfLinks {
    my $self = shift;
    return PowerLineModule::getNumberOfLinks( $self->{_dimmer} );
}

sub printLinkTable
{    #nothing to print until after startGatherLinkTable/getNumberOfLinks
    my $self = shift;
    return PowerLineModule::printLinkTable( $self->{_dimmer} );
}

sub extendedGet {
    my $self = shift;
    return PowerLineModule::extendedGet( $self->{_dimmer}, 1 );
}

sub printExtendedGet {    #nothing to print until after call to extendedGet
    my $self = shift;
    return PowerLineModule::printExtendedGet( $self->{_dimmer}, 1 );
}

sub linkPlm {
    my $self = shift;
    my $amController = shift;
    my $group = shift;
    my $ls1 = shift;
    my $ls2 = shift;
    my $ls3 = shift;
    return PowerLineModule::linkPlm( $self->{_dimmer}, $amController,
   	$group, $ls1, $ls2, $ls3 );
}

sub unLinkPlm {
    my $self = shift;
    my $amController = shift;
    my $group = shift;
    my $ls3 = shift;
    return PowerLineModule::unLinkPlm( $self->{_dimmer}, 
    	$amController, $group, $ls3);
}

sub createDeviceLink {
    my $self      = shift;
    my $responder = shift;
    my $group     = shift;
    my $ls1       = shift;
    my $ls2       = shift;
    my $ls3       = shift;
    return PowerLineModule::createDeviceLink( $self->{_dimmer},
        $responder->{_dimmer}, $group, $ls1, $ls2, $ls3 );
}

sub removeDeviceLink {
    my $self      = shift;
    my $responder = shift;
    my $group     = shift;
    my $ls3       = shift;
    return PowerLineModule::removeDeviceLink( $self->{_dimmer},
        $responder->{_dimmer}, $group, $ls3 );
}

sub createModemGroupToMatch {
    my $self  = shift;
    my $group = shift;
    return PowerLineModule::createModemGroupToMatch( $group, $self->{_dimmer} );
}
1;
