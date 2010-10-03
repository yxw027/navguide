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

# Object files
lib_obj:= match.o util.o key.o
test_obj:= sift-test.o

# Definitions
DEFS = sift.h

# ------------------------ Rules --------------------------------
static_lib:=../../lib/libsift.a
test:=sift-test

CXXFLAGS := $(CFLAGS_NOOPT) $(CFLAGS_GLIB) $(CFLAGS_IPP)  \
		 -Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
		 -ffast-math -msse2 -Wno-deprecated
LDFLAGS =  $(LDFLAGS_STD) `pkg-config --libs glib-2.0 gthread-2.0` $(LDFLAGS_LCM) $(LIBS_COMMON) $(LDFLAGS_IPP) $(LDFLAGS_OPENCV) -lsift -lcommon $(LDFLAGS_LCMTYPES) $(LDFLAGS_BOT_CORE)

####### Implicit rules

.SUFFIXES: .cpp .cxx .cc .C .c

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: $(static_lib) $(test)

$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

$(test): $(test_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(test_obj) $(LDFLAGS) -lpthread

clean:
	-rm -f $(static_lib) $(shared_lib) $(lib_obj)

# Implicit rule used by Gnu Make: $(CC) -c $(CPPFLAGS) $(CFLAGS)
${OBJ}: ${DEFS}


