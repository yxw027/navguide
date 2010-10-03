include ../common.mk

CC = gcc

CFLAGS = -I.. $(CFLAGS_IPP) $(CFLAGS_MKL) $(CFLAGS_GRAPHVIZ) $(CFLAGS_COMMON) $(CFLAGS_LCM) $(CFLAGS_GTK) \
	 -DMODELS_DIR='"$(MODELS_DIR)"' $(CFLAGS_BOT) $(CFLAGS_OPENCV) \
	 -DCONFIG_DIR='"$(CONFIG_DIR)"' -fPIC

LDFLAGS = $(LDFLAGS_STD) $(LDFLAGS_COMMON) $(LDFLAGS_LCM) $(LDFLAGS_GTK) $(LDFLAGS_OPENCV)\
		  $(LDFLAGS_GL) $(LDFLAGS_BOT_CORE) -lguidance -lcommon -limage -ljpegcodec $(LDFLAGS_LCMTYPES) $(LDFLAGS_IPP) $(LDFLAGS_GRAPHVIZ)  \
		  $(LDFLAGS_MKL) -lbot-viewer -lbot-gtk -ljpeg -lsift

VIEWER_OBJS = udp_util.o \
			  util.o \
			  main.o \
	renderer_lcmgl.o \
	renderer_image.o \
	renderer_guidance.o \
	renderer_scrolling_plots.o

VIEWER=$(BIN_PATH)/nv-viewer
VIEWER_DEPENDENCIES:=$(VIEWER_OBJS)

all: $(VIEWER)

%.o: %.c %.h
	@echo "    [$@]"
	$(CC) $(CFLAGS) -c $< 

.cpp.o:
	$(CXX) -c $(CFLAGS) $(INCPATH) -o $@ $<

$(VIEWER): $(VIEWER_DEPENDENCIES)
	$(CC) -o $@ $(VIEWER_OBJS) $(LDFLAGS)

clean:
	rm -f *.o *.a *~ $(VIEWER)
