include ../common.mk

# Indicates compilator to use
CC      = g++

# Specifies compilator options
CFLAGS  := -O3 -Wall $(CFLAGS_OPENCV) -D LINUX $(INCPATH) $(CFLAGS_GLIB)
LDFLAGS := $(LDFLAGS_LCMTYPES) $(LDFLAGS_OPENCV) $(LDFLAGS_GLIB)

# Files extensions .cpp, .o
SUFFIXES = .cpp .o 
.SUFFIXES: $(SUFFIXES) .

# Name of the main program
SURF_PROG  = surf
SURF_LIB   = ../../lib/libsurf.a

# Object files .o necessary to build the main program
OBJS_LIB  = fasthessian.o integral.o main.o surf.o utils.o ipoint.o
OBJS  = $(OBJS_LIB) main.o
 
all: $(PROG) $(SURF_LIB)

# Compilation and link
$(SURF_LIB): $(OBJS_LIB)
	@echo "    [$@]"
	ar rc $@ $(OBJS_LIB)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS)

.cpp.o:
	$(CC)   $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(PROG)
	-rm -f *.o
