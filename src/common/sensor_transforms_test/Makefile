sensor_transforms_test := sensor_transforms_test
OBJECTS = sensor_transforms_test.o

CFLAGS := -Wall -g -O2 -std=gnu99 -I../..
CC := gcc

all: $(sensor_transforms_test)

sensor_transforms_test: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) ../../../lib/libcommon.a -lgsl -lgslcblas

clean:
	rm -f $(OBJECTS) $(sensor_transforms_test)
