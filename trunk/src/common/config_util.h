#ifndef _CONFIG_UTIL_H__
#define _CONFIG_UTIL_H__

#include "config.h"
#include "dbg.h"
#include "codes.h"
#include <assert.h>
#include <glib.h>
#include <lcmtypes/navlcm_features_param_t.h>

typedef struct {
    int platform;
    int nsensors;
    int nbuckets;
    char **channel_names;
    char **unit_ids;
    int features_max_delay_sec;
    double features_scale_factor;
    int features_feature_type;

    double tracker_max_dist;
    double tracker_add_min_dist;
    double tracker_second_neighbor_ratio;

    char *data_dir;
    int sift_enabled;
    int fast_enabled;
    int gftt_enabled;

    // sift parameters
    int sift_doubleimsize;
    int sift_nscales;
    double sift_sigma;
    double sift_peakthresh;

    // gftt parameters
    double gftt_quality;
    double gftt_min_dist;
    int gftt_block_size;
    int gftt_use_harris;
    double gftt_harris_param;

    // surf params
    int surf_octaves;
    int surf_intervals;
    int surf_init_sample;
    double surf_threshold;

    // fast parameters
    int fast_threshold;

    // upward camera intrinsic calibration
    double upward_calib_cc[2];
    double upward_calib_fc[2];
    double upward_calib_kc[5];
    
    // classifier calibration
    char *classcalib_filename;
    double *classcalib;

} config_t;

config_t* read_config_file ();


#endif
