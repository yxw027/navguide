include ../common.mk

guidance:=../../bin/nv-guidance
guidance_lib:=../../lib/libguidance.a

CFLAGS =  $(INCPATH) `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_CAMUNITS) $(CFLAGS_IPP) $(CFLAGS_OPENCV) $(CFLAGS_MKL) $(CFLAGS_CONFIG_DIR) $(CFLAGS_GRAPHVIZ)

#CFLAGS = $(INCPATH) $(CFLAGS_NOOPT) $(CFLAGS_GLIB) \
	$(CFLAGS_GLADE) $(CFLAGS_OPENCV) $(CFLAGS_IPP) $(CFLAGS_MKL) \
	$(CFLAGS_JPEGCODEC) -I$(PROJECT_HOME)/src \
	$(CFLAGS_GRAPHVIZ) \
	$(CFLAGS_CAMUNITS) $(CFLAGS_LCM) $(CFLAGS_CONFIG_DIR)\
	-Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
	-ffast-math -msse2 -Wno-deprecated -DMALLOC_CHECK=3
	 
LDFLAGS = $(LDFLAGS_STD) $(LDFLAGS_GTK) $(LDFLAGS_GTK_UTIL) \
-lfeatures	-lmser -lfast -lsift2 -lsurf -lcommon -limage $(LDFLAGS_GRAPHVIZ) \
	$(LDFLAGS_LCM) $(LDFLAGS_LCMTYPES) $(LDFLAGS_IPP) \
        -ljpegcodec -limage -lsift -lglut $(LDFLAGS_OPENCV) $(LDFLAGS_MKL) -lgsl $(LDFLAGS_BOT_CORE)

guidance_lib_obj:= dijkstra.o loop.o classifier.o state.o tracker.o bags.o rotation.o corrmat.o util.o matcher.o flow.o
guidance_obj:= $(guidance_lib_obj) main.o

.PHONY: all test clean tidy

all: $(guidance) $(guidance_lib)

$(guidance): $(guidance_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(guidance_obj) $(LDFLAGS) -lpthread

$(guidance_lib): $(guidance_lib_obj)
	@echo "    [$@]"
	ar rc $@ $(guidance_lib_obj)

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo guidance : TODO

clean: tidy
	rm -f $(guidance) $(guidance_obj) $(guidance_lib) $(guidance_lib_obj)

tidy:
	rm -f $(guidance_obj)
