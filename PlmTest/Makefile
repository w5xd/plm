INCLUDES = -I/usr/local/include -I../api
CC_FLAGS = -g -O2 -pthread -c -fPIC -DPIC -DLINUX32

OBJECTS = PlmTest.o

all: PlmTest ../bin/PlmTest

clean:
	rm *.o PlmTest --force ../bin/PlmTest

PlmTest.o: PlmTest.c ../api/PowerLineModule.h

.c.o:
	g++ -x c $(CC_FLAGS) $(INCLUDES) $< -o $@

PlmTest: $(OBJECTS) ../lib/libw5xdInsteon.so 
	g++ -fPIC -DPIC  $(OBJECTS) -O2 -pthread  -L../lib -o PlmTest -lw5xdInsteon -lboost_thread

../bin/PlmTest: PlmTest 
	rm -f ../bin/PlmTest
	ln PlmTest ../bin

