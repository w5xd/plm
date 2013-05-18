SUBS = impl PlmTest

.PHONY:	all $(SUBS) PowerLineModule

all:	impl PlmTest PowerLineModule

PlmTest:	impl

PowerLineModule:	impl

$(SUBS):
	cd $@ && make

PowerLineModule:
	cd PowerLineModule && /usr/local/bin/perl Makefile.PL
	cd PowerLineModule && make

