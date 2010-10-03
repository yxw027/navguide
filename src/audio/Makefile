include ../common.mk

nv-audio:=../../bin/nv-audio

CFLAGS =  `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_CONFIG_DIR) `pkg-config --cflags gstreamer-0.10` 

LDFLAGS = $(LDFLAGS_STD)  `pkg-config --libs glib-2.0 gthread-2.0` ../../lib/libcommon.a $(LDFLAGS_LCM) $(LDFLAGS_LCMTYPES) $(LIBS_COMMON) $(LDFLAGS_GTK) `pkg-config --libs gstreamer-0.10` 


nv-audio_obj:=main.o

.PHONY: all test clean tidy

all: $(nv-audio)

$(nv-audio): $(nv-audio_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(nv-audio_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo nv-audio : TODO

clean: tidy
	rm -f $(nv-audio)

tidy:
	rm -f $(nv-audio_obj)
