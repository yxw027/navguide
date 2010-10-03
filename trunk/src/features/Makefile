include ../common.mk

features:=../../bin/nv-features
features_lib =../../lib/libfeatures.a

CFLAGS =  `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_CAMUNITS) $(CFLAGS_IPP) $(CFLAGS_OPENCV)
LDFLAGS =  $(LDFLAGS_STD) `pkg-config --libs glib-2.0 gthread-2.0` $(LDFLAGS_LCM) $(LIBS_COMMON) $(LDFLAGS_IPP) $(LDFLAGS_OPENCV) -lsift -lsift2 -lmser -lfast -limage -lcommon -ljpegcodec -lsurf -lcommon $(LDFLAGS_LCMTYPES) $(LDFLAGS_BOT_CORE)


features_lib_obj:= util.o
features_obj:= $(features_lib_obj) main.o

.PHONY: all test clean tidy

all: $(features) $(features_lib)

$(features): $(features_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(features_obj) $(LDFLAGS) -lpthread

$(features_lib): $(features_lib_obj)
	@echo "    [$@]"
	ar rc $@ $(features_lib_obj)

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo features : TODO

clean: tidy
	rm -f $(features) $(features_lib) 

tidy:
	rm -f $(features_obj) $(features_lib_obj)
