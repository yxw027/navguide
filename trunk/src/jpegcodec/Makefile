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
lib_obj:= colorcomp.o dechtbl.o decoder.o decqtbl.o enchtbl.o encoder.o encqtbl.o imgload.o jpegbase.o jpegload.o metadata.o pngload.o ppmload.o

# ------------------------ Rules --------------------------------
static_lib:=../../lib/libjpegcodec.a

CXXFLAGS := $(INCPATH) $(CFLAGS_NOOPT) $(CFLAGS_GTK) $(CFLAGS_GLIB) $(CFLAGS_GLADE) $(CFLAGS_IPP) \
		$(CFLAGS_JPEGCODEC) -I$(PROJECT_HOME)/src -I$(SRC_ROOT)/libpmk-1.5 \
		 -Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
		 -ffast-math -msse2 -Wno-deprecated

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

all: $(static_lib)

$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

clean:
	-rm -f $(static_lib) $(shared_lib) $(lib_obj)

# Implicit rule used by Gnu Make: $(CC) -c $(CPPFLAGS) $(CFLAGS)
${OBJ}: ${DEFS}

