#ifndef _TRACKER2_H__
#define _TRACKER2_H__

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_feature_match_set_t.h>
#include <lcmtypes/navlcm_feature_list_t.h>
#include <lcmtypes/navlcm_track_set_t.h>

/* from common */
#include <common/codes.h>
#include <common/lcm_util.h>
#include <common/mkl_math.h>

#include <glib.h>
#include <guidance/matcher.h>

struct track2_t { GQueue *data; };

struct tracker2_data_t { GQueue *data; float **frontmat; int **frontmat_total; gboolean frontmat_training; int nsensors;};

int track_init (navlcm_feature_t *f);
int tracker_data_update (tracker2_data_t *t, navlcm_feature_list_t *f, int nsensors);
int tracker_data_init (tracker2_data_t *tracks);
int tracker_data_init_from_data (tracker2_data_t *tracks, navlcm_feature_list_t *f);
int tracker_data_printout (tracker2_data_t *tracks);

navlcm_track_set_t *track_list_to_lcm (tracker2_data_t *tracks);

#endif

