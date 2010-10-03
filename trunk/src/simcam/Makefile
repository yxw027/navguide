include ../common.mk

simcam:=../../bin/simcam

CFLAGS += $(INCPATH) $(CFLAGS_NOOPT) $(CFLAGS_GTK) $(CFLAGS_GLIB) $(CFLAGS_GLADE) $(CFLAGS_IPP) \
	$(CFLAGS_JPEGCODEC) $(CFLAGS_REMOTE_DIR) $(CFLAGS_CONFIG_DIR) -I$(PROJECT_HOME)/src \
	 -I$(SRC_ROOT)/libpmk-1.5 \
	$(CFLAGS_CAMUNITS) $(CFLAGS_LCM) $(CFLAGS_JPEGCODEC) \
	$(CFLAGS_GRAPHVIZ) \
	-Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
	-ffast-math -msse2 -Wno-deprecated

LDFLAGS += $(LDFLAGS_GTK) $(LDFLAGS_DC1394) $(LDFLAGS_GTK_UTIL) $(LDFLAGS_CAMUNITS) $(LDFLAGS_IPP) \
	$(LDFLAGS_GRAPHVIZ) $(LDFLAGS_GLADE)  $(LDFLAGS_OPENGL) $(LDFLAGS_LCM)  \
	-lcommon $(LDFLAGS_LCMTYPES) -lglut 

simcam_obj:=main.o

.PHONY: all test clean tidy

all: $(simcam)

$(simcam): $(simcam_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(simcam_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo simcam : TODO

clean: tidy
	rm -f $(simcam)

tidy:
	rm -f $(simcam_obj)
