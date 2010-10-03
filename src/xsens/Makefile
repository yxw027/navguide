include ../common.mk

xsens:=../../bin/nv-xsens

CFLAGS =  `pkg-config --cflags glib-2.0` -I.. 
LDFLAGS = $(LDFLAGS_STD)  `pkg-config --libs glib-2.0 gthread-2.0` $(LIBS_COMMON)  $(LDFLAGS_LCMTYPES) $(LDFLAGS_LCM)


xsens_obj:=main.o xsens.o

.PHONY: all test clean tidy

all: $(xsens)

$(xsens): $(xsens_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(xsens_obj) $(LDFLAGS) -lpthread

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo xsens : TODO

clean: tidy
	rm -f $(xsens)

tidy:
	rm -f $(xsens_obj)
