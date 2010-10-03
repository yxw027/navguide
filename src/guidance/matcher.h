#ifndef _MATCHER_H__
#define _MATCHER_H__

/* From IPP */
#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippcc.h"
#include "ippj.h"

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_track_set_t.h>
#include <lcmtypes/navlcm_feature_match_set_t.h>
#include <lcmtypes/navlcm_feature_list_t.h>

/* from image */
#include <image/image.h>

/* from common */
#include <common/codes.h>
#include <common/lcm_util.h>
#include <common/mkl_math.h>

#include <glib.h>

#define EDGE_TOP 0
#define EDGE_BOTTOM 1
#define EDGE_RIGHT 2
#define EDGE_LEFT 3

#define MATCHING_NCC 0
#define MATCHING_DOTPROD 1

struct track_t { int id; int start, end; int *idx; double *col, *row; int *sid; 
                         unsigned char desc[128]; };

class tracker_t {
 public:

    Ipp8u *prev, *next;
    Ipp32f *prev_32f, *next_32f;
    Ipp32f *prev_mean_32f, *next_mean_32f;
    
    int width, height;
    int *pts;
    int npts;

    tracker_t ();
    ~tracker_t ();
    
    void init (int w, int h);
    void copy_data (unsigned char *data, int w, int h, int radius);
    void ncc_run_straight_calibration (int nx, int ny, int radius, int search_radius);       
    void ncc_run (int col, int row, int radius, int search_radius);
    void search_pointer ();
    void ncc_publish (lcm_t *lcm, int sensor_id);
    void compute_mean_image (Ipp32f *src, Ipp32f *dst, int width, int height, int radius);
    void ncc_filter_tracks (float ratio);

};


navlcm_feature_match_set_t*
find_feature_matches_multi (navlcm_feature_list_t *keys1, 
                            navlcm_feature_list_t *keys2, gboolean within_camera, gboolean across_cameras,
                            int topK, double thresh, double maxdist, 
                            double max_dist_ft, gboolean monogamy);
int
find_feature_matches_fast (navlcm_feature_list_t *keys1, 
                            navlcm_feature_list_t *keys2, gboolean within_camera,
                            gboolean across_cameras, gboolean monogamy, gboolean mutual_consistency,
                            double thresh, double maxdist, int matching_mode,
                            navlcm_feature_match_set_t *matches);

void match_sanity_check (
        navlcm_feature_match_set_t *matches,
        navlcm_feature_list_t *set1, navlcm_feature_list_t *set2, 
        gboolean within_camera, gboolean across_cameras);
gboolean feature_on_edge (double col, double row, int width, int height, double maxdist);

navlcm_feature_match_set_t*
filter_matches_polygamy (navlcm_feature_match_set_t *matches, gboolean within_camera);

int track_find_closest_index (track_t tr, double col, double row, int sensorid, double *dist);
navlcm_feature_match_set_t filter_feature_matches (navlcm_feature_match_set_t *m, float ratio);
int neighbor_features (float col1, float row1, int sid1, 
                       float col2, float row2, int sid2,
			           int width, int height,
                       int ncells, int max_cell_dist);
double features_min_distance (navlcm_feature_t *f1, GList *features);
int feature_edge_code (double col, double row, int width, int height);
unsigned char edge_to_index (navlcm_feature_t *f, int width, int height);

#endif
