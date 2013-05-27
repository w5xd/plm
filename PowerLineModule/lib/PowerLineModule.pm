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
our %EXPORT_TAGS = ( 'all' => [ qw(
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
);

our $VERSION = '0.01';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.

    my $constname;
    our $AUTOLOAD;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "&PowerLineModule::constant not defined" if $constname eq 'constant';
    my ($error, $val) = constant($constname);
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
XSLoader::load('PowerLineModule', $VERSION);

# Preloaded methods go here.
{
	package PowerLineModule::Dimmer;
		require PowerLineModule;
		sub new {
			my $proto = shift;
			my $class = ref($proto) || $proto;
			my $self = { _dimmer => shift	};
			bless $self, $class;
			return $self;
		}
		sub setValue {
			my $self = shift;
			my $res = PowerLineModule::setDimmerValue($self->{_dimmer}, shift);
		}
		sub getValue {
			my $self = shift;
			return PowerLineModule::getDimmerValue($self->{_dimmer}, shift);
		}
		sub readX10Code {
			my $self = shift;
			delete $self->{_x10House};
			delete $self->{_x10Unit};
			my @dX10 = PowerLineModule::getX10Code($self->{_dimmer});
			if ($dX10[0] >= 0) {
				$self->{_x10House} = $dX10[1];
				$self->{_x10Unit} = $dX10[2];
			}
			return $dX10[0];
		}
		sub x10House { #returns undef until readX10Code is called
			my $self = shift;
			return $self->{_x10House};
		}
		sub x10Unit {
			my $self = shift;
			return $self->{_x10Unit};
		}
		sub startGatherLinkTable {
			my $self = shift;
			return PowerLineModule::startGatherLinkTable($self->{_dimmer});
		}
		sub getNumberOfLinks {
			my $self = shift;
			return PowerLineModule::getNumberOfLinks($self->{_dimmer});
		}
		sub printLinkTable {
			my $self = shift;
			return PowerLineModule::printLinkTable($self->{_dimmer});
		}
		sub extendedGet {
			my $self = shift;
			return PowerLineModule::extendedGet($self->{_dimmer}, 1);
		}
		sub printExtendedGet {
			my $self = shift;
			return PowerLineModule::printExtendedGet($self->{_dimmer}, 1);
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
			return PowerLineModule::setWallLEDbrightness($self->{_keypad}, shift);
		}
		sub extendedGet {
			my $self = shift;
			return PowerLineModule::extendedGet($self->{_keypad}, shift);
		}
		sub printExtendedGet {
			my $self = shift;
			return PowerLineMOdule::printExtendedGet($self->{_keypad}, shift);
		}

	package PowerLineModule::Modem;
	require PowerLineModule;
	sub new {
		my $class = shift;
		my $self = {
			_commPort => shift,
			_level => shift,
			_logfileName => shift,
		};
		my @res = PowerLineModule::openPowerLineModem(
			$self->{_commPort}, $self->{_level}, $self->{_logfileName});
		$self->{_modem} = $res[0];
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
	sub getDimmerAccess {
		my $self = shift;
                my $dimmer = PowerLineModule::getDimmerAccess($self->{_modem}, shift);
		if ($dimmer ne 0) {
			return PowerLineModule::Dimmer->new($dimmer);}
		else {return 0;}
	}
	sub getKeypadAccess {
		my $self = shift;
		my $keypad = PowerLineModule::getKeypadAccess($self->{_modem}, shift);
		if ($keypad ne 0) {
			return PowerLineModule::Keypad->new($keypad); }
		else {return 0;}
	}
	sub getModemLinkRecords {
		my $self = shift;
		return PowerLineModule::getModemLinkRecords($self->{_modem});
	}
	sub printLinkTable {
		my $self = shift;
		return PowerLineModule::printModemLinkTable($self->{_modem});
	}
}

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

PowerLineModule - Perl extension that reads/writes insteon powerline modem

=head1 SYNOPSIS

  use PowerLineModule;
  blah blah blah

=head1 DESCRIPTION

Stub documentation for PowerLineModule, created by h2xs. It looks like the
author of the extension was negligent enough to leave the stub
unedited.

Blah blah blah.

=head2 EXPORT

None by default.

=head2 Exportable functions

  Dimmer getDimmerAccess(Modem modem, const char *addr)
  Modem openPowerLineModem(const char *commPortName)


=head1 SEE ALSO

Mention other useful documentation such as the documentation of
related modules or operating system documentation (such as man pages
in UNIX), or any relevant external documentation such as RFCs or
standards.

If you have a mailing list set up for your module, mention it here.

If you have a web site set up for your module, mention it here.

=head1 AUTHOR

ww, E<lt>ww@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Wayne Wright, W5XD

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.


=cut
