include ../common.mk

collector:=../../bin/nv-collector

CFLAGS = $(INCPATH) $(CFLAGS_NOOPT) $(CFLAGS_GLIB)  \
	  $(CFLAGS_LCM) \
	-Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
	-ffast-math -msse2 -Wno-deprecated

LDFLAGS = $(LDFLAGS_STD) $(LDFLAGS_LCM) -lcommon $(LDFLAGS_LCMTYPES) $(LDFLAGS_GLIB) $(LDFLAGS_GTK) $(LDFLAGS_BOT_CORE)

collector_obj:= main.o

.PHONY: all test clean tidy

all: $(collector)

$(collector): $(collector_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(collector_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo collector : TODO

clean: tidy
	rm -f $(collector)

tidy:
	rm -f $(collector_obj)
