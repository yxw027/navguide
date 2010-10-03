ROOT_PATH=$(shell pwd)/../..
SRC_ROOT:= $(ROOT_PATH)/src
LIB_PATH:=$(ROOT_PATH)/lib
BIN_PATH:=$(ROOT_PATH)/bin
BASH_PATH:=$(ROOT_PATH)/bash
CC := g++
INCPATH:= -I. -I.. -I$(SRC_ROOT)
CFLAGS_CXX := -g -Wall -O2 -I.. -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wno-unused
CFLAGS_NOOPT :=  -g -Wall -I.. -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wno-unused
CFLAGS_STD   := $(CFLAGS_ARG) -std=gnu99 -g \
	-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE \
	-Wall -Wno-unused-parameter -Wno-format-zero-length

#LIB_DIR:=$(shell pwd)/../../lib
LIB_DIR:=../../lib
LIBS_COMMON:= -lcommon
LDFLAGS_STD := -L$(LIB_PATH)

PROJECT_HOME:=$(shell pwd)/../..

#LIBS_COMMON:=-lm -lpthread -lcommon -ltypes -llc
#LIBS_COMMON:=-lpthread $(LIB_DIR)/libcommon.a -ltypes -llc

ifneq "$(shell uname -s)" "Darwin"
    RFLAGS_COMMON := -Wl,-R$(shell pwd)/../../lib
endif
#LDFLAGS := -L$(LIB_DIR) $(RFLAGS_COMMON) $(LIBS_COMMON)
#LDFLAGS := -L$(LIB_DIR) $(RFLAGS_COMMON)
LDFLAGS_COMMON_SHARED := -L$(LIB_DIR) $(RFLAGS_COMMON) -lpthread 

# default config options
ENABLE_FFMPEG:=0
ENABLE_DC1394:=1
ENABLE_SURF:=0

# config overrides
#-include ../common.options

CONFIG_DIR:=$(shell pwd)/../../config
CFLAGS_CONFIG_DIR:=-DCONFIG_DIR='"$(CONFIG_DIR)"'
CFLAGS_BIN_DIR:=-DBIN_DIR='"$(BIN_PATH)"'
CFLAGS_BASH_DIR:=-DBASH_DIR='"$(BASH_PATH)"'

#DATA_DIR:=$(shell pwd)/../../data
#CFLAGS_DATA_DIR:=-DDATA_DIR='"$(DATA_DIR)"'

# graphviz
CFLAGS_GRAPHVIZ:= `pkg-config --cflags libgvc`
LDFLAGS_GRAPHVIZ:= `pkg-config --libs libgvc`
# glade target path should be relative to one directory deep from common.mk
GLADE_TARGET_PATH:=$(shell pwd)/../../bin/glade/
CFLAGS_GLADE:=-DGLADE_TARGET_PATH='"$(GLADE_TARGET_PATH)"' \
	`pkg-config --cflags libglade-2.0`
LDFLAGS_GLADE:=`pkg-config --libs libglade-2.0` -rdynamic

# JPEG codec
CFLAGS_JPEGCODEC:=-I$(PROJECT_HOME)/src/jpegcodec

# ffmpeg (avi codecs and libraries)
ifeq ($(ENABLE_FFMPEG),1)
    CFLAGS_FFMPEG:=`pkg-config --cflags libavformat libavcodec` -DENABLE_FFMPEG
    LDFLAGS_FFMPEG:=`pkg-config --libs libavformat libavcodec`
endif

# Intel Integrated Performance Primitives
IPPA:=
IPP_LIBS:=-lguide -lippcore -lippi -lippcc -lipps -lippj -lippcv
ifeq "$(shell uname -s)" "Darwin"
    IPP_BASE:=/Library/Frameworks/Intel_IPP.framework
    CFLAGS_IPP:=-I$(IPP_BASE)/Headers
    LDFLAGS_IPP:=-L$(IPP_BASE)/Libraries $(IPP_LIBS)
else
    ifeq "$(shell uname -m)" "x86_64"
        IPP_SEARCH := /usr/local/intel/ipp/5.1/em64t \
                      /opt/intel/ipp/5.2/em64t       \
		      /opt/intel/ipp/5.3/em64t       \
		      /opt/intel/ipp/6.0.2.076/em64t \
		      /opt/intel/ipp/6.1.0.039/em64t
        test_dir = $(shell [ -e $(dir) ] && echo $(dir))
        IPP_SEARCH := $(foreach dir, $(IPP_SEARCH), $(test_dir))
        IPP_BASE := $(firstword $(IPP_SEARCH))
        IPP_LIBS:=-lguide -lippcoreem64t -lippiem64t -lippjem64t -lippccem64t -lippcvem64t -lpthread -lippsem64t
        IPPA:=em64t
    else
        IPP_SEARCH :=  	/opt/intel/ipp/5.3.2.068/ia32 \
			/opt/intel/ipp/5.3.4.080/ia32 \
			/opt/intel/ipp/6.0.2.076/ia32 \
			/opt/intel/ipp/5.3.3.075/ia32
        test_dir = $(shell [ -e $(dir) ] && echo $(dir))
        IPP_SEARCH := $(foreach dir, $(IPP_SEARCH), $(test_dir))
        IPP_BASE := $(firstword $(IPP_SEARCH))
    endif
    CFLAGS_IPP:=-I$(IPP_BASE)/include
    LDFLAGS_IPP:=-L$(IPP_BASE)/sharedlib -Wl,-R$(IPP_BASE)/sharedlib $(IPP_LIBS)
endif

# Intel Math Kernel Library (MKL)
ifeq "$(shell uname -m)" "x86_64"
MKL_LIBS := -lmkl -lmkl_core -lmkl_intel_thread -lguide
MKL_SEARCH := /opt/intel/mkl/10.1.3.027 \
			  /opt/intel/mkl/10.2.1.017
test_dir = $(shell [ -e $(dir) ] && echo $(dir))
MKL_SEARCH := $(foreach dir, $(MKL_SEARCH), $(test_dir))
MKL_BASE := $(firstword $(MKL_SEARCH))
CFLAGS_MKL := -I$(MKL_BASE)/include
LDFLAGS_MKL := -L$(MKL_BASE)/lib/em64t -Wl,-R$(MKL_BASE)/lib/em64t $(MKL_LIBS)
else
MKL_LIBS := -lmkl -lmkl_core -lmkl_intel_thread -lguide
MKL_SEARCH := /opt/intel/cmkl/10.1.3.027
test_dir = $(shell [ -e $(dir) ] && echo $(dir))
MKL_SEARCH := $(foreach dir, $(MKL_SEARCH), $(test_dir))
MKL_BASE := $(firstword $(MKL_SEARCH))
CFLAGS_MKL := -I$(MKL_BASE)/include
LDFLAGS_MKL := -L$(MKL_BASE)/lib/32 -Wl,-R$(MKL_BASE)/lib/32 $(MKL_LIBS)
endif

# Intel Compiler
ICC_SEARCH := /opt/intel/Compiler/11.0/083
test_dir = $(shell [ -e $(dir) ] && echo $(dir))
ICC_SEARCH := $(foreach dir, $(ICC_SEARCH), $(test_dir))
ICC_BASE := $(firstword $(ICC_SEARCH))
CFLAGS_ICC := -I$(ICC_BASE)/include
LDFLAGS_ICC := -L$(ICC_BASE)/lib/ia32 -liomp5 -lpthread -lm

# libdc1394
ifeq ($(ENABLE_DC1394),1)
    CFLAGS_DC1394:=-DENABLE_DC1394
    ifeq "$(shell uname -s)" "Darwin"
	LDFLAGS_DC1394:=-ldc1394 -L/usr/local/lib
    else
	LDFLAGS_DC1394:=-ldc1394 -L/usr/local/lib -Wl,-R/usr/local/lib
    endif
endif

ifeq ($(ENABLE_SURF),1)
    CFLAGS_SURF := -DENABLE_SURF
    LDFLAGS_SURF := $(LIB_DIR)/libSurf.a 
endif

# jpeg
ifeq "$(shell test -f /usr/lib/libjpeg-ipp.so -o -f /usr/lib64/libjpeg-ipp.so && echo ipp)" "ipp"
        LDFLAGS_JPEG := -ljpeg-ipp
else
        LDFLAGS_JPEG := -ljpeg
endif

# gtk
CFLAGS_GTK:=`pkg-config --cflags gtk+-2.0`
LDFLAGS_GTK:=`pkg-config --libs gtk+-2.0`

# glib
CFLAGS_GLIB := `pkg-config --cflags glib-2.0 gmodule-2.0`
LDFLAGS_GLIB := `pkg-config --libs glib-2.0 gmodule-2.0 gthread-2.0`

# OpenCV
CFLAGS_OPENCV := `pkg-config --cflags opencv`
LDFLAGS_OPENCV := `pkg-config --libs opencv`

ifeq "$(shell uname -s)" "Darwin"
#LDSH := -dynamiclib
	LDSH := -dynamic
	SHEXT := .dylib
else
	LD := gcc
	LDSH := -shared
	SHEXT := .so
endif

# OpenGL
CFLAGS_OPENGL :=
LDFLAGS_OPENGL := -lGL -lGLU

# GNU scientific libraries
CFLAGS_GSL := -DHAVE_INLINE `gsl-config --cflags`
LDFLAGS_GSL := `gsl-config --libs`

# internal libraries
LDFLAGS_GTK_UTIL := $(LIB_DIR)/libgtk_util.a

# Open GL
CFLAGS_GL    :=
LDFLAGS_GL   := -lGLU -lGLU -lglut

# libbot
BOT_INCLUDE_DIR := /usr/local/include
CFLAGS_BOT := -I$(BOT_INCLUDE_DIR)  -I$(BOT_INCLUDE_DIR)/bot/lcmtypes 
LDFLAGS_BOT_CORE := -lbot-core

# camunits
CFLAGS_CAMUNITS:= -I/usr/local/include
LDFLAGS_CAMUNITS:=`pkg-config --libs camunits camunits-gtk`

# GPU SIFT
CFLAGS_GPUSIFT:=-I$(SRC_ROOT)/SiftGPU/Include -Wall -pthread
LDFLAGS_GPUSIFT:= -lCg -lCgGL -lGLEW -lglut

#LCM
CFLAGS_LCM:=-I/usr/local/include/
LDFLAGS_LCM:= -llcm

#LCMTYPES
LDFLAGS_LCMTYPES:= $(LIB_DIR)/liblcmtypes.a

