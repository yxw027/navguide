include ../common.mk

# ------------------------ Rules --------------------------------
static_lib:=../../lib/libcommon.a

lib_obj:=  ppm.o fileio.o gltool.o config.o config_util.o texture.o serial.o ioutils.o timestamp.o mathutil.o getopt.o dgc_vector.o hashtable.o glib_util.o date.o lcm_util.o  applanix.o quaternion.o mkl_math.o sysinfo.o globals.o camtrans.o navconf.o atrans.o udp_util.o stringutil.o

CXXFLAGS := $(CFLAGS_NOOPT) $(CFLAGS_GTK) $(CFLAGS_GLIB) $(CFLAGS_IPP) $(CFLAGS_LCM) $(CFLAGS_MKL)\
		-Wno-multichar -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
		 -ffast-math -msse2 -Wno-deprecated -DCONFIG_DIR='"$(CONFIG_DIR)"'

CFLAGS_ICC := $(CFLAGS_GLIB)

####### Implicit rules

.SUFFIXES: .cpp .cxx .cc .C .c

all: $(static_lib)
	
#mkl_math.o: mkl_math.cpp
#	icc -c $(CFLAGS_ICC) $(INCPATH) -o $@ $<

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

####### Build rules

$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

clean:
	-rm -f $(static_lib) $(shared_lib) $(lib_obj)
