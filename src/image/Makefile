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
lib_obj:= image.o

# Definitions
DEFS = image.h

# ------------------------ Rules --------------------------------
static_lib:=../../lib/libimage.a

CXXFLAGS := -Wno-deprecated $(CFLAGS_NOOPT) $(CFLAGS_GLIB) $(CFLAGS_IPP) \
	$(CFLAGS_GTK) \
	 -Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
	 -ffast-math -msse2 

IPPLIBS := -lippj$(LIB_ARCH) -lippcc$(LIB_ARCH) -lippi$(LIB_ARCH) \
	-lipps$(LIB_ARCH) -lippcore$(LIB_ARCH)

####### Implicit rules

.SUFFIXES: .cpp .cxx .cc .C .c

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(IPPFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: $(static_lib)

$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

clean:
	-rm -f $(static_lib) $(shared_lib) $(lib_obj) *~

# Implicit rule used by Gnu Make: $(CC) -c $(CPPFLAGS) $(CFLAGS)
${OBJ}: ${DEFS}


