package PowerLineModule;

use 5.010001;
use strict;
use warnings;
use Carp;

require Exporter;
use AutoLoader;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use PowerLineModule ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = (
    'all' => [
        qw(
          )
    ]
);

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
);

our $VERSION = '0.01';

sub AUTOLOAD {

    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.

    my $constname;
    our $AUTOLOAD;
    ( $constname = $AUTOLOAD ) =~ s/.*:://;
    croak "&PowerLineModule::constant not defined" if $constname eq 'constant';
    my ( $error, $val ) = constant($constname);
    if ($error) { croak $error; }
    {
        no strict 'refs';

        # Fixed between 5.005_53 and 5.005_61
        #XXX	if ($] >= 5.00561) {
        #XXX	    *$AUTOLOAD = sub () { $val };
        #XXX	}
        #XXX	else {
        *$AUTOLOAD = sub { $val };

        #XXX	}
    }
    goto &$AUTOLOAD;
}

require XSLoader;
XSLoader::load( 'PowerLineModule', $VERSION );

# Preloaded methods go here.
{
    package PowerLineModule::Dimmer;
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

    sub x10House {       #need to have previously called extendedGet
        my $self = shift;
	$self->_readX10Code();
        return $self->{_x10House};
    }

    sub x10Unit {	#need to have previously called extendedGet
        my $self = shift;
	$self->_readX10Code();
        return $self->{_x10Unit};
    }

    sub setX10Code {
	my $self = shift;
	my $hc = shift;
	my $unit = shift;
	return PowerLineModule::setX10Code( $self->{_dimmer}, $hc, $unit);
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

    sub linkAsController {
	my $self = shift;
	return PowerLineModule::linkAsController( $self->{_dimmer}, shift);
    }

    sub unLinkAsController {
	my $self = shift;
	return PowerLineModule::unLinkAsController( $self->{_dimmer}, shift);
    }

    sub createDeviceLink {
	my $self = shift;
	my $responder = shift;
	my $group = shift;
	my $ls1 = shift; my $ls2 = shift; my $ls3 = shift;
	return PowerLineModule::createDeviceLink( $self->{_dimmer},
						$responder->{_dimmer},
						$group, $ls1, $ls2, $ls3);
    }

    sub removeDeviceLink {
	my $self = shift;
	my $responder = shift;
	my $group = shift;
	my $ls3 = shift;
	return PowerLineModule::removeDeviceLink( $self->{_dimmer}, $responder->{_dimmer}, $group, $ls3);
    }

    sub createModemGroupToMatch {
	my $self = shift;
	my $group = shift;
	return PowerLineModule::createModemGroupToMatch($group, $self->{_dimmer});
    }

    package PowerLineModule::Keypad;
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

    package PowerLineModule::Modem;
    require PowerLineModule;

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
        if ( $dimmer ne 0 ) {
            return PowerLineModule::Dimmer->new($dimmer);
        }
        else { return 0; }
    }

    sub getKeypad {
        my $self = shift;
        my $keypad = PowerLineModule::getKeypadAccess( $self->{_modem}, shift );
        if ( $keypad ne 0 ) {
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
	my $self = shift; my $group = shift; my $val = shift;
	return PowerLineModule::setAllDevices( $self->{_modem}, $group, $val);
    }

    sub getNextUnusedControlGroup {
	my $self = shift;
	return PowerLineModule::getNextUnusedControlGroup( $self->{_modem});
    }

    sub deleteGroupLinks {
	my $self = shift;
	my $group = shift;
	return PowerLineModule::deleteGroupLinks( $self->{_modem}, $group);
    }
}

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__


=head1 NAME

PowerLineModule - Perl extension that reads/writes insteon powerline modem

=head1 SYNOPSIS

  require PowerLineModule;
  my $modem = PowerLineModule::Modem->new("COM1", 0, "MyLogFile.txt");
  #or, for Linux, /dev/ttyS0 instead of COM1
  if ($modem->openOk())
  {
  	my $dimmer = $modem->getDimmer("11.22.33"); #11.22.33 is insteon ID
	my $keypad = $modem->getKeypad("22.33.44"); #insteon ID for keypadLinc

	$dimmer->setValue(255); #set to full brightness
	$keypad->setWallLEDbrightness(64); #0 to 64
  }

=head1 DESCRIPTION

Interface to the insteon PLM (power line module) serial port.

=head2 EXPORT

None by default.

=head2 Exportable functions

=head1 SEE ALSO

see https://github.com/w5xd/plm.git

=head1 AUTHOR

Wayne Wright, w5xd@alum.mit.edu

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Wayne Wright, W5XD

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.


=cut
