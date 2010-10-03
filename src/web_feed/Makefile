include ../common.mk

web_feed:=../../bin/nv-web_feed

CFLAGS +=  `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_CAMUNITS) $(CFLAGS_IPP) $(CFLAGS_OPENCV)
LDFLAGS +=  `pkg-config --libs glib-2.0 gthread-2.0` $(LDFLAGS_LCM) $(LIBS_COMMON) $(LDFLAGS_IPP) $(LDFLAGS_OPENCV) -lcommon -limage -ljpegcodec $(LDFLAGS_LCMTYPES)


web_feed_obj:= main.o

.PHONY: all test clean tidy

all: $(web_feed) $(web_feed_lib)

$(web_feed): $(web_feed_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(web_feed_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo web_feed : TODO

clean: tidy
	rm -f $(web_feed) $(web_feed_lib) 

tidy:
	rm -f $(web_feed_obj) $(web_feed_lib_obj)
