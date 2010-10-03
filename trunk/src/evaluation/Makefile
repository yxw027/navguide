include ../common.mk

evaluation:=../../bin/nv-evaluation-tool

CFLAGS = $(INCPATH) $(CFLAGS_NOOPT) $(CFLAGS_GTK) $(CFLAGS_GLIB) $(CFLAGS_GLADE) $(CFLAGS_IPP) $(CFLAGS_MKL) \
	$(CFLAGS_CONFIG_DIR) -I$(PROJECT_HOME)/src \
	$(CFLAGS_LCM) $(CFLAGS_CAMUNITS)

LDFLAGS = $(LDFLAGS_STD) $(LDFLAGS_GTK) $(LDFLAGS_GTK_UTIL) $(LDFLAGS_IPP) \
	$(LDFLAGS_GLADE)  $(LDFLAGS_LCM) $(LDFLAGS_OPENCV) $(LDFLAGS_BOT_CORE) \
	-lglut -lguidance -lfeatures -limage -lcommon -lsurf -lsift -lsift2 -lfast -lmser -ljpegcodec $(LDFLAGS_LCMTYPES) $(LDFLAGS_MKL)

evaluation_obj:= events.o draw.o util.o main.o

.PHONY: all test clean tidy

gladefiles:=../../bin/glade/evaluation.glade

all: $(evaluation) $(gladefiles)

../../bin/glade/%.glade: %.glade

$(evaluation): $(evaluation_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(evaluation_obj) $(LDFLAGS) -lpthread

../../bin/glade/%.glade: %.glade
	@echo "    [$@]"
	cp -f $< $@

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo evaluation : TODO

clean: tidy
	rm -f $(evaluation)

tidy:
	rm -f $(evaluation_obj)
