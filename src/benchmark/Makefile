PROG = benchmark

CC = g++

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
#    	CFLAGS +=  `pkg-config --cflags glib-2.0`  -I/opt/intel/mkl/10.1.2.024/include/
    	CFLAGS +=  `pkg-config --cflags glib-2.0` 
# MKL
#    	LDFLAGS +=   `pkg-config --libs glib-2.0 gthread-2.0` -L/opt/intel/mkl/10.1.2.024/lib/64 -lmkl -lmkl_core -lmkl_intel_lp64 -lmkl_intel_thread -lguide -lpthread
# libblas
#     	LDFLAGS +=   `pkg-config --libs glib-2.0 gthread-2.0` -lcblas -lpthread
# Atlas
	LDFLAGS +=   `pkg-config --libs glib-2.0 gthread-2.0` -L/usr/lib64/atlas -L/usr/lib/atlas/lib -L/usr/lib -L/usr/lib/gcc/x86_64-linux-gnu/4.2.4 -lblas -latlas -lgfortran
else
    	CFLAGS +=  `pkg-config --cflags glib-2.0`  -I/opt/intel/mkl/10.1.2.024/include/
# MKL
#    	LDFLAGS +=   `pkg-config --libs glib-2.0 gthread-2.0` -L/opt/intel/mkl/10.1.2.024/lib/32 -lmkl -lmkl_core -lmkl_intel -lmkl_intel_thread -lguide -lpthread
# Atlas
    	LDFLAGS +=   `pkg-config --libs glib-2.0 gthread-2.0` -L/usr/lib/atlas/lib/ -latlas -lblas -lguide -lpthread
endif


PROG_OBJ:=main.o

.PHONY: all test clean tidy

all: $(PROG)

$(PROG): $(PROG_OBJ)
	@echo "    [$@]"
	$(CC) -o $@ $(PROG_OBJ) $(LDFLAGS)

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

test: 
	@echo benchmark : TODO

clean: tidy
	rm -f $(PROG)

tidy:
	rm -f $(PROG_OBJ)
