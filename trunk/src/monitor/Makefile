include ../common.mk

monitor:=../../bin/nv-monitor

CFLAGS +=  `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_GLIB) $(CFLAGS_BASH_DIR)

LDFLAGS =   $(LDFLAGS_STD) `pkg-config --libs glib-2.0 gthread-2.0` $(LDFLAGS_LCM) $(LIBS_COMMON) $(LDFLAGS_GTK) $(LDFLAGS_LCMTYPES) $(LDFLAGS_BOT_CORE)


monitor_obj:=main.o

.PHONY: all test clean tidy

all: $(monitor)

$(monitor): $(monitor_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(monitor_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo monitor : TODO

clean: tidy
	rm -f $(monitor)

tidy:
	rm -f $(monitor_obj)
