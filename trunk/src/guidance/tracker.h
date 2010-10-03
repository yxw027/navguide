#ifndef _TRACKER_H__
#define _TRACKER_H__

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_feature_match_set_t.h>
#include <lcmtypes/navlcm_feature_list_t.h>
#include <lcmtypes/navlcm_track_set_t.h>

/* from common */
#include <common/codes.h>
#include <common/lcm_util.h>

#include <glib.h>

/* From here */
#include <guidance/classifier.h>

/* From GSL */
#include <gsl/gsl_sort.h>
#include <gsl/gsl_statistics.h>

#define FATE_TOP_LEFT 1
#define FATE_TOP_RIGHT 2
#define FATE_BOTTOM_RIGHT 3
#define FATE_BOTTOM_LEFT 4
#define FATE_RIGHT 5
#define FATE_LEFT 6

#define FATE_PASS 1
#define FATE_FAIL 2

navlcm_track_set_t *init_tracks ();
navlcm_track_set_t* reset_tracks (navlcm_track_set_t *tracks);
navlcm_track_set_t *update_tracks (navlcm_track_set_t *tracks,
                                     navlcm_feature_list_t *features,
                                     int ttl, int nbuckets, double tracker_max_dist, GList **dead_tracks);
navlcm_track_set_t *add_new_tracks (navlcm_track_set_t *tracks, navlcm_feature_list_t *features, int ttl, double param_min_dist);
navlcm_track_set_t *fuse_tracks (navlcm_track_set_t *tracks, int ttl, int width, int height, lcm_t *lcm);

void publish_tracks (lcm_t *lcm, navlcm_track_set_t *tracks);
navlcm_track_set_t *remove_dead_tracks (navlcm_track_set_t *tracks,
                                          navlcm_track_set_t **dead_tracks);
void compute_track_median (navlcm_track_t *track, double *median, double *lowerq, double *upperq);
double* compute_track_mean (navlcm_track_t *track);
double *compute_track_stdev (navlcm_track_t *track, double *mean);
void compute_track_stat (navlcm_track_t *track, int width, int height);
int track_n_crossings (navlcm_track_t *track);
navlcm_track_t *track_random (int length, int ftsize, int ncrossings, int64_t reftime,
                                int width, int height);
int track_path_code (navlcm_track_t *track);
void track_print_features (navlcm_track_t *track, const char *filename);
double track_overlap (navlcm_track_t *t1, navlcm_track_t *t2);
GList *track_split (navlcm_track_t *track);
navlcm_feature_list_t* track_filter_on_utime (navlcm_feature_list_t *fs, int64_t utime);

navlcm_feature_list_t* tracks_to_features (GList *ts, int width, int height, int sensorid, int64_t utime, int dist_mode);

#endif
