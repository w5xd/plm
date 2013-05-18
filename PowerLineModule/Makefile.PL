use 5.010001;
use ExtUtils::MakeMaker;

my $LocalLibArgs;
my $LocalDefineArgs;

if ($^O =~ /win/i ) {
    $LocalLibArgs = "-L../Release";
    $LocalDefineArgs = '-DWIN32';
} else {
    $LocalLibArgs = "-L../lib";
    $LocalDefineArgs = '-DLINUX32';
 };

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    NAME              => 'PowerLineModule',
    VERSION_FROM      => 'lib/PowerLineModule.pm', # finds $VERSION
    PREREQ_PM         => {}, # e.g., Module::Name => 1.1
    ($] >= 5.005 ?     ## Add these new keywords supported since 5.005
      (ABSTRACT_FROM  => 'lib/PowerLineModule.pm', # retrieve abstract from module
       AUTHOR         => 'Wayne Wright <w5xd@alum.mit.edu>') : ()),
    LIBS              => $LocalLibArgs. ' -lw5xdInsteon -lboost_thread', 
    DEFINE            => $LocalDefineArgs,
    INC               => '-I.  -I../api', 
	# Un-comment this if you add C files to link with later:
    # OBJECT            => '$(O_FILES)', # link all the C files too
);
if  (eval {require ExtUtils::Constant; 1}) {
  # If you edit these definitions to change the constants used by this module,
  # you will need to use the generated const-c.inc and const-xs.inc
  # files to replace their "fallback" counterparts before distributing your
  # changes.
  my @names = (qw());
  ExtUtils::Constant::WriteConstants(
                                     NAME         => 'PowerLineModule',
                                     NAMES        => \@names,
                                     DEFAULT_TYPE => 'IV',
                                     C_FILE       => 'const-c.inc',
                                     XS_FILE      => 'const-xs.inc',
                                  );

}
else {
  use File::Copy;
  use File::Spec;
  foreach my $file ('const-c.inc', 'const-xs.inc') {
    my $fallback = File::Spec->catfile('fallback', $file);
    copy ($fallback, $file) or die "Can't copy $fallback to $file: $!";
  }
}
