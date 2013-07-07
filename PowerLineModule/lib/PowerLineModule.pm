# Copyright (c) 2013 by Wayne Wright, Round Rock, Texas.
# See license at http://github.com/w5xd/plm/blob/master/LICENSE.md
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

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__

=head1 NAME

PowerLineModule - Perl wrapper for insteon powerline modem w5xdInsteon.dll

=head1 SYNOPSIS

  require PowerLineModule;
  my $diagnosticsLevel = 1;
  my $modem = PowerLineModule::Modem->new("COM1", $diagnosticsLevel, "MyLogFile.txt");
  #or, for Linux, /dev/ttyS0 instead of COM1
  if ($modem->openOk())
  {
  	my $dimmer = $modem->getDimmer("11.22.33"); #11.22.33 is insteon ID
	my $keypad = $modem->getKeypad("22.33.44"); #insteon ID for keypadLinc

	$dimmer->setValue(255); #set to full brightness
	$keypad->setWallLEDbrightness(64); #0 to 64
  }

  #deal with events...
  sub insteonEvent { #...
  	}
   
   $dimmer->monitorCb(\&insteonEvent); #bind dimmer to sub address
   my $waitSeconds = 1;
   while (1) {
      $modem->monitor($waitSeconds); #calls insteonEvent
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

Copyright (C) 2013 by Wayne Wright, W5XD

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.


=cut

