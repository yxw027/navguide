include ../common.mk

LDFLAGS+=`pkg-config --libs glib-2.0 camunits lcm` -llcmtypes
CFLAGS+=`pkg-config --cflags glib-2.0 camunits lcm` -g -O2 -Wall

targets:=resize_and_publish listen_for_key_events print_example
objects:=resize_and_publish.o listen_for_key_events.o print_example.o

all: $(targets)

print_example: print_example.o
	$(CC) -o $@ $< $(LDFLAGS)

resize_and_publish: resize_and_publish.o
	$(CC) -o $@ $< $(LDFLAGS)

listen_for_key_events: listen_for_key_events.o
	$(CC) -o $@ $< $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(targets) $(objects)
