include ../common.mk

CC = gcc

CFLAGS = -I.. $(CFLAGS_STD) $(CFLAGS_COMMON) $(CFLAGS_LCM) $(CFLAGS_GTK) \
	 -DMODELS_DIR='"$(MODELS_DIR)"' $(CFLAGS_BOT) \
	 -DCONFIG_DIR='"$(CONFIG_DIR)"' -fPIC

LDFLAGS = $(LDFLAGS_STD) $(LDFLAGS_COMMON) $(LDFLAGS_LCM) $(LDFLAGS_GTK) \
		  $(LDFLAGS_GL) $(LDFLAGS_BOT_CORE) \
		  -lbot-viewer -lbot-gtk -ljpeg -lcommon

VIEWER_OBJS = udp_util.o \
			  main.o \
	renderer_lcmgl.o \
	renderer_image.o \

VIEWER=$(BIN_PATH)/nv-viewer
VIEWER_DEPENDENCIES:=$(VIEWER_OBJS)

all: $(VIEWER)

%.o: %.c %.h
	@echo "    [$@]"
	$(CC) $(CFLAGS) -c $< 

$(VIEWER): $(VIEWER_DEPENDENCIES)
	$(CC) -o $@ $(VIEWER_OBJS) $(LDFLAGS)

clean:
	rm -f *.o *.a *~ $(VIEWER)
