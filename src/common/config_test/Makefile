config_test := config_test
OBJECTS = config_test.o

CFLAGS := -Wall -g -O2 -std=gnu99 -I../..
CC := gcc

all: $(config_test)

config_test: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) ../../../lib/libcommon.a

clean:
	rm -f $(OBJECTS) $(config_test)
