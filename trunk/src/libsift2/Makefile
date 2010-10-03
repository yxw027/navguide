include ../common.mk

# file:        Makefile
# author:      Andrea Vedaldi
# description: Build SIFT++

NAME               := siftpp
VER                := 0.7.4
DIST                = $(NAME)-$(VER)
BINDIST             = $(DIST)-$(ARCH)

# --------------------------------------------------------------------
#                                                       Error messages
# --------------------------------------------------------------------

err_no_arch  =
err_no_arch +=$(shell echo "** Unknown host architecture '$(UNAME)'. This identifier"   1>&2)
err_no_arch +=$(shell echo "** was obtained by running 'uname -sm'. Edit the Makefile " 1>&2)
err_no_arch +=$(shell echo "** to add the appropriate configuration."                   1>&2)
err_no_arch +=Configuration failed

test_obj := sift-test.o
lib_obj := sift.o sift-driver.o

static_lib:=../../lib/libsift2.a
test:=sift-test

# --------------------------------------------------------------------
#                                                                Flags
# --------------------------------------------------------------------
# Options:
# LOWE_STRICT - Be more similar to Lowe's version
# USEFASTMATH - Use faster approximated computations

CXXFLAGS           += -I. -Wall -g -O3 -DNDEBUG -Werror -Wno-unused
CXXFLAGS           += -Wno-variadic-macros
CXXFLAGS           += -DVL_LOWE_STRICT 
CXXFLAGS           += -DVL_USEFASTMATH
CXXFLAGS           += -I$(SRC_ROOT)

# Determine on the flight the system we are running on
Darwin_PPC_ARCH    := mac
Darwin_i386_ARCH   := mci
Linux_i386_ARCH    := glx
Linux_i686_ARCH    := glx
Linux_x86_64_ARCH  := g64
Linux_unknown_ARCH := glx

UNAME             := $(shell uname -sm)
ARCH              := $($(shell echo "$(UNAME)" | tr \  _)_ARCH)

ifeq ($(ARCH),)
die:=$(error $(err_no_arch))
endif

mac_CXXFLAGS       := -DVL_MAC
mci_CXXFLAGS       := -DVL_MAC
glx_CXXFLAGS       :=
g64_CXXFLAGS       :=

CXXFLAGS           += $($(ARCH)_CXXFLAGS)
LDFLAGS =  $(LDFLAGS_STD) `pkg-config --libs glib-2.0 gthread-2.0` $(LDFLAGS_LCM) $(LIBS_COMMON) $(LDFLAGS_IPP) $(LDFLAGS_OPENCV) -lsift2 -lcommon $(LDFLAGS_LCMTYPES) $(LDFLAGS_BOT_CORE)

# --------------------------------------------------------------------
#
# --------------------------------------------------------------------


all: $(static_lib) $(test)

clean:
	-rm -f $(static_lib) $(lib_obj) $(test_obj) $(test)

.SUFFIXES: .cpp .cxx .cc .C .c

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(CFLAGS_GLIB) $(INCPATH) -o $@ $<


$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

$(test): $(test_obj)
	@echo "    [$@]"
	$(CC) -o $@ $(test_obj) $(LDFLAGS) -lpthread

