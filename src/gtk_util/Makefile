SRC_ROOT:=..
LIB_PATH:=../../lib

include ../common.mk

CFLAGS= $(CFLAGS_STD) $(INCPATH) $(CFLAGS_GTK) -g

static_lib:=$(LIB_PATH)/libgtk_util.a
shared_lib:=$(LIB_PATH)/libgtk_util.so

LDFLAGS_TESTER:=$(LDFLAGS_GTK) $(static_lib) $(LDFLAGS)

lib_obj:=gtk_util.o param_widget.o \
       glx_context.o gtkgldrawingarea.o user_control.o gtkarrowbin.o \
       fbgldrawingarea.o


param_widget_tester:=param_widget_tester
rgb_canvas_tester:=rgb_canvas_tester
ezavi_button_tester:=ezavi_button_tester
gtkgl_tester:=gtkgl_tester

param_widget_tester_obj:=param_widget_tester.o
rgb_canvas_tester_obj:=rgb_canvas_tester.o
ezavi_button_tester_obj:=ezavi_button_tester.o
gtkgl_tester_obj:=gtkgl_tester.o

#$(shared_lib) 
targets:=$(static_lib) 

ifeq "$(shell uname -s)" "Darwin"
	LDFLAGS_LIB += -L../../lib -lcommon
endif

all: $(targets)
	
$(static_lib): $(lib_obj)
	@echo "    [$@]"
	ar rc $@ $(lib_obj)

param_widget_tester: $(param_widget_tester_obj) $(static_lib)
	@echo "    [$@]"
	gcc -o $@ $(param_widget_tester_obj) $(LDFLAGS_TESTER)

rgb_canvas_tester: $(rgb_canvas_tester_obj) $(static_lib)
	@echo "    [$@]"
	gcc -o $@ $(rgb_canvas_tester_obj) $(LDFLAGS_TESTER)

ezavi_button_tester: $(ezavi_button_tester_obj) $(static_lib)
	@echo "    [$@]"
	gcc -o $@ $(ezavi_button_tester_obj) $(LDFLAGS_TESTER)

gtkgl_tester: $(gtkgl_tester_obj) $(static_lib)
	@echo "    [$@]"
	gcc -o $@ $(gtkgl_tester_obj) $(LDFLAGS_TESTER) -lglut

%.o: %.c %.h
	@echo "    [$@]"
	gcc -c -o $@ $< $(CFLAGS) 

%.o: %.cpp
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

%.o: %.c
	@echo "    [$@]"
	gcc -c -o $@ $< $(CFLAGS) 

.PHONY: clean
clean: tidy
	rm -f $(targets) $(rgb_canvas_tester) *~ *.o

.PHONY: tidy
tidy:
	rm -f $(lib_obj) $(param_widget_tester_obj) $(rgb_canvas_tester_obj) \
	    $(ezavi_button_tester_obj) $(gtkgl_tester_obj)
