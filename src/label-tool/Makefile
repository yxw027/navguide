include ../common.mk

label_tool:=../../bin/nv-label-tool

CFLAGS = $(INCPATH) $(CFLAGS_NOOPT) $(CFLAGS_GTK) $(CFLAGS_GLIB) $(CFLAGS_GLADE) $(CFLAGS_LCM) $(CFLAGS_BOT_CORE) $(CFLAGS_OPENCV)

LDFLAGS = $(LDFLAGS_STD) $(LDFLAGS_GTK) $(LDFLAGS_GTK_UTIL) $(LDFLAGS_GLADE)  $(LDFLAGS_LCM) $(LDFLAGS_BOT_CORE) $(LDFLAGS_OPENCV) -lcommon -lbot-gl -lbot-gtk

label_tool_obj:=label-tool.o

gladefiles:=../../bin/glade/label-tool.glade

all: $(label_tool) $(gladefiles)

../../bin/%.glade: %.glade

$(label_tool): $(label_tool_obj)
	@echo "		[$@]"
	$(CC) -o $@ $(label_tool_obj) $(LDFLAGS) 

../../bin/glade/%.glade: %.glade
	@echo "    [$@]"
	cp -f $< $@

%.o: %.c
	@echo "    [$@]"
	$(CC) -c -o $@ $< $(CFLAGS) 

clean: tidy
	rm -f $(label_tool)

tidy:
	rm -f $(label_tool_obj)

