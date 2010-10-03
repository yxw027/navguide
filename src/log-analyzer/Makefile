include ../common.mk

log-analyzer:=../../bin/nv-log-analyzer
log-analyzer_lib:=../../lib/liblog-analyzer.a

CFLAGS =  $(INCPATH) `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_CAMUNITS) $(CFLAGS_IPP) $(CFLAGS_OPENCV) $(CFLAGS_MKL) $(CFLAGS_CONFIG_DIR) $(CFLAGS_GRAPHVIZ)

LDFLAGS = $(LDFLAGS_STD) $(LDFLAGS_GTK) $(LDFLAGS_GTK_UTIL) \
-lfeatures	-lmser -lfast -lsift2 -lsurf -lguidance -lcommon -limage $(LDFLAGS_GRAPHVIZ) \
	$(LDFLAGS_LCM) $(LDFLAGS_LCMTYPES) $(LDFLAGS_IPP) \
        -ljpegcodec -limage -lsift -lglut $(LDFLAGS_OPENCV) $(LDFLAGS_MKL) -lgsl $(LDFLAGS_BOT_CORE)

log-analyzer_obj:= main.o

.PHONY: all test clean tidy

all: $(log-analyzer)

$(log-analyzer): $(log-analyzer_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(log-analyzer_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo log-analyzer : TODO

clean: tidy
	rm -f $(log-analyzer) $(log-analyzer_obj) 

tidy:
	rm -f $(log-analyzer_obj)
