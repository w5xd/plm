INCLUDES = -I/usr/local/include -I..
APACHE_DEFINES = -D_REENTRANT -D_GNU_SOURCE
CC_FLAGS = -O2 -pthread -c -fPIC -DPIC -DLINUX32 -std=c++11

OBJECTS = PlmMonitor.o PlmMonitorMap.o PlmMonitorLinux.o PowerLineModule.o \
    Dimmer.o X10Dimmer.o InsteonDevice.o Keypad.o Fanlinc.o

all: clean ../lib/libw5xdInsteon.so ../bin/w5xdInsteon.so

clean:
	rm -f *.o ../lib/libw5xdInsteon.so ../bin/w5xdInsteon.so

PlmMonitor.o: PlmMonitor.cpp PlmMonitor.h Dimmer.h Keypad.h

Fanlinc.o: Fanlinc.cpp Fanlinc.h

PlmMonitorLinux.o: PlmMonitorLinux.cpp PlmMonitorLinux.h PlmMonitor.h

PlmMonitorMap.o: PlmMonitorMap.cpp PlmMonitor.h PlmMonitorMap.h 

Dimmer.o: Dimmer.cpp Dimmer.h PlmMonitor.h

Keypad.o: Keypad.cpp Keypad.h Dimmer.h InsteonDevice.h

Dimmer.h: InsteonDevice.h

X10Dimmer.o: X10Dimmer.cpp X10Dimmer.h PlmMonitor.h

PowerLineModule.o: PowerLineModule.cpp ../api/PowerLineModule.h

.cpp.o:
	g++ $(CC_FLAGS) $(INCLUDES) $< -o $@

../bin/w5xdInsteon.so: $(OBJECTS)
	mkdir -p ../bin
	g++ -shared  -fPIC -DPIC  $(OBJECTS) -O2 -pthread -Wl,-soname -Wl,w5xdInsteon.so -o ../bin/w5xdInsteon.so -lboost_thread

../lib/libw5xdInsteon.so: ../bin/w5xdInsteon.so
	mkdir -p ../lib
	rm -f ../lib/libw5xdInsteon.so
	ln ../bin/w5xdInsteon.so ../lib/libw5xdInsteon.so
