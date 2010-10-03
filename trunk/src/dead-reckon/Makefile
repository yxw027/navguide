dead_reckon:=../../bin/nv-dead_reckon

include ../common.mk

CFLAGS =  `pkg-config --cflags glib-2.0` -I.. 
LDFLAGS = $(LDFLAGS_STD)  `pkg-config --libs glib-2.0 gthread-2.0` $(LIBS_COMMON)  $(LDFLAGS_LCMTYPES) $(LDFLAGS_LCM) $(LDFLAGS_BOT_CORE)

.PHONY: all test clean tidy

all: $(dead_reckon)

dead_reckon_obj := dead_reckon.o 

$(dead_reckon): $(dead_reckon_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(dead_reckon_obj) $(LDFLAGS)

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) -lpthread

test: 
	@echo TODO: test dead_reckon

clean: tidy
	rm -f $(dead_reckon)

tidy:
	rm -f $(dead_reckon_obj)

