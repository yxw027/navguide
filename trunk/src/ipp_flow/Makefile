include ../common.mk

# ------------------ Compilation options ------------------------

# Loads math library.  
# LIBS = -lm

# Flags for the C compiler:
#   -Wall for strict gcc warnings (requires prototypes for all functions).
#   -g to produce debug data for gdb
#   -O for optimization
# CFLAGS = -Wall -g -O

# CC = g++

# --------------------- Code modules ----------------------------

LIB_PATH:=../../lib

# Object files
lib_obj:= flow.o

# Definitions
DEFS = flow.h

CFLAGS := $(CFLAGS_IPP) \
		 -Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
		 -ffast-math -msse2

LDFLAGS := $(LDFLAGS_IPP) -lpthread $(LDFLAGS)

# ------------------------ Rules --------------------------------
static_lib:=../../lib/libippflow.a

####### Implicit rules

.SUFFIXES: .cpp .cxx .cc .C .c

.cpp.o:
	$(CXX) -c $(CFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: $(static_lib)

$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

clean:
	-rm -f $(static_lib) $(shared_lib) $(lib_obj)

# Implicit rule used by Gnu Make: $(CC) -c $(CPPFLAGS) $(CFLAGS)
${OBJ}: ${DEFS}


