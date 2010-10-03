CC=gcc
CFLAGS=-Wall -g -I../..

OBJECTS=test-parse.o gen-google-map.o
EXECS=test-parse gen-google-map

all: $(EXECS)

test-parse: test-parse.o
	$(CC) -o $@ $^ ../../../lib/libcommon.a
	
gen-google-map: gen-google-map.o
	$(CC) -o $@ $^ ../../../lib/libcommon.a

clean:
	-rm -f $(OBJECTS) $(EXECS)
