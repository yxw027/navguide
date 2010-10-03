include ../common.mk

logger:=../../bin/nv-logger
logconverter:=../../bin/nv-log-converter

CFLAGS +=  `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_LOG_DIR) -g -O2 -g -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT -Wno-unused-parameter -Wshadow

LDFLAGS +=  `pkg-config --libs glib-2.0 gthread-2.0` ../../lib/libcommon.a  $(LDFLAGS_LCMTYPES) $(LDFLAGS_GTK) $(LDFLAGS_LCM) $(LDFLAGS_BOT_CORE)


logger_obj:= logger.o
logconverter_obj:= log-converter.o

.PHONY: all test clean tidy

all: $(logger) $(logconverter)

$(logger): $(logger_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(logger_obj) $(LDFLAGS) -lpthread

$(logconverter): $(logconverter_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(logconverter_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo logger : TODO

clean: tidy
	rm -f $(logger) $(logconverter)

tidy:
	rm -f $(logger_obj) $(logconverter_obj)
