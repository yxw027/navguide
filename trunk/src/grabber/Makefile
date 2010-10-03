include ../common.mk

grabber:=../../bin/nv-grabber

CFLAGS =  `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_CAMUNITS) $(CFLAGS_IPP) $(CFLAGS_CONFIG_DIR)

LDFLAGS = $(LDFLAGS_STD)  `pkg-config --libs glib-2.0 gthread-2.0` $(LDFLAGS_LCM) $(LDFLAGS_COMMON) $(LDFLAGS_CAMUNITS) $(LDFLAGS_IPP) $(LDFLAGS_OPENGL) -lcommon $(LDFLAGS_LCMTYPES) $(LDFLAGS_BOT_CORE)


grabber_obj:=main.o

.PHONY: all test clean tidy

all: $(grabber)

$(grabber): $(grabber_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(grabber_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo grabber : TODO

clean: tidy
	rm -f $(grabber)

tidy:
	rm -f $(grabber_obj)
