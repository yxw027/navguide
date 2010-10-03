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
lib_obj:= mser.o

# ------------------------ Rules --------------------------------
static_lib:=../../lib/libmser.a

CXXFLAGS := $(CFLAGS_NOOPT) $(CFLAGS_GLIB) $(CFLAGS_IPP)  \
		 -Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
		 -Wno-deprecated
####### Implicit rules

.SUFFIXES: .cpp .cxx .cc .C .c

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: $(static_lib)

$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

clean:
	-rm -f $(static_lib) $(lib_obj)

# Implicit rule used by Gnu Make: $(CC) -c $(CPPFLAGS) $(CFLAGS)
${OBJ}: ${DEFS}


