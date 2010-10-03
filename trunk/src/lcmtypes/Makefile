include ../common.mk

CFLAGS = $(CFLAGS_LCM) -I.. -O4 -fPIC
LDFLAGS = $(LDFLAGS_LCM)

LCMTYPES = audio_param_t  feature_list_t       feature_t       nav_order_t      s60_key_event_t \
class_param_t  feature_match_set_t  gps_to_local_t  phone_command_t  tablet_event_t \
dictionary_t   feature_match_t      imu_t           mser_list_t     phone_param_t    track_set_t \
double_list_t  features_param_t     mser_t          phone_print_t    track_t 	system_info_t	generic_cmd_t \
ui_map_t	ui_map_node_t	ui_map_edge_t	 logger_info_t	flow_t	key_byte_t image_metadata_old_t	 image_old_t 	

CAMLCM_TYPES = key_string_t 

# prevent auto-generated lcm .c/.h files from being deleted
.SECONDARY : $(LCMTYPES:navlcm_%.c) $(LCMTYPES:navlcm_%.h) \
	         $(CAMLCM_TYPES:camlcm_%.c) $(CAMLCM:camlcm_%.h) 

LCMTYPES_O=$(LCMTYPES:%=navlcm_%.o) $(CAMLCM_TYPES:%=camlcm_%.o)

LCM = lcm-gen
LIBLCMTYPES = $(LIB_PATH)/liblcmtypes.a

all: $(LIBLCMTYPES) navlcm.h

navlcm.h: Makefile
	@echo >  $@ "#ifndef __navlcm_h__"
	@echo >> $@ "#define __navlcm_h__"
	@for f in $(LCMTYPES:%=navlcm_%.h); do \
	    echo >> $@ "#include <lcmtypes/$$f>" || exit 2; done
	@echo >> $@ "#endif"
all: $(LIBLCMTYPES)

$(LIBLCMTYPES): $(LCMTYPES_O)
	ar rc $@ $(LCMTYPES_O)

%.o: %.c %.h
	@echo "    [$@]"
	gcc $(CFLAGS) -c $< 

navlcm_%.c navlcm_%.h: ../../lcmtypes/%.lcm
	@echo "    [$@]"
	$(LCM) -c $<

camlcm_%.c camlcm_%.h: ../../lcmtypes/%.lcm
	@echo "    [$@]"
	$(LCM) -c $<

clean:
	rm -f *.c *.h *.o *.a *~ $(LIBLCMTYPES)

#$(targets) $(objects)
