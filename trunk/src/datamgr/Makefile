include ../common.mk

datamgr:=../../bin/nv-datamgr

CFLAGS =  `pkg-config --cflags glib-2.0` -I.. $(CFLAGS_LCM) $(CFLAGS_GLIB) $(CFLAGS_BIN_DIR)

LDFLAGS = $(LDFLAGS_STD)  `pkg-config --libs glib-2.0 gthread-2.0` $(LDFLAGS_LCM) $(LIBS_COMMON) $(LDFLAGS_GTK) $(LDFLAGS_LCMTYPES)


datamgr_obj:=main.o

.PHONY: all test clean tidy

all: $(datamgr)

$(datamgr): $(datamgr_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(datamgr_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo datamgr : TODO

clean: tidy
	rm -f $(datamgr)

tidy:
	rm -f $(datamgr_obj)
