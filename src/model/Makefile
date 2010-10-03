include ../common.mk

# ------------------ Compilation options ------------------------

# Loads math library.  
# LIBS = -lm

# Flags for the C compiler:
#   -Wall for strict gcc warnings (requires prototypes for all functions).
#   -g to produce debug data for gdb
#   -O for optimization
# CFLAGS = -Wall -g -O

# --------------------- Code modules ----------------------------
static_lib:=../../lib/libmodel.a

CFLAGS:= $(CFLAGS) $(CFLAGS_GLIB)
LDFLAGS:=$(LDFLAGS_GLIB) $(LIBS_COMMON)
LDFLAGS_TESTER:=$(static_lib) $(LDFLAGS)

# Object files
lib_obj:= model.o

model_tester:=model_tester
model_tester_obj:=model_tester.o

# ------------------------ Rules --------------------------------

CXXFLAGS := $(CFLAGS_NOOPT) $(CFLAGS_GLIB)  \
		 -Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
		 -ffast-math -msse2 -Wno-deprecated
####### Implicit rules

####### Build rules

all: $(static_lib)

$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

model_tester: $(model_tester_obj) $(static_lib)
	@echo "    [$@]"
	gcc -o $@ $(model_tester_obj) $(LDFLAGS_TESTER) -lglut

clean:
	-rm -f $(static_lib) $(shared_lib) $(lib_obj) $(model_tester_obj)

# Implicit rule used by Gnu Make: $(CC) -c $(CPPFLAGS) $(CFLAGS)
${OBJ}: ${DEFS}


