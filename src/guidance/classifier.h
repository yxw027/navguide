#ifndef _CLASSIFIER_H__
#define _CLASSIFIER_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_feature_match_set_t.h>

/* from common */
#include <common/dbg.h>
#include <common/mathutil.h>
#include <common/fileio.h>
#include <common/glib_util.h>
#include <common/applanix.h>
#include <common/quaternion.h>
#include <common/config_util.h>
#include <common/date.h>
#include <bot/bot_core.h>

/* from here */
#include <guidance/rotation.h>

/* from matcher */
#include "matcher.h"

#define CLASS_SET_CALIB(i, j, m, s, val) ((m)[(i)*(s)+(j)] = (val))
#define CLASS_GET_CALIB(i, j, m, s) ((m)[(i)*(s)+(j)])

typedef struct {
    int nbins;
    int *hist;
    double range_min;
    double range_max;
} classifier_hist_t;

typedef struct {
    
    int size; // pdf over motion templates
    double *pdf;

    int hist_size; // history of recognized motions
    int *motion1;
    int *motion2;
} motion_classifier_t;

double class_compute_imu_rotation (navlcm_imu_t *p1, navlcm_imu_t *p2);
double class_compute_pose_rotation (botlcm_pose_t *p1, botlcm_pose_t *p2);
int class_orientation (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2, config_t *config, double *angle_rad, double *variance, lcm_t *lcm);
void classifier_smooth_class_states (GQueue *states);
double class_calibration_dead_reckoning (int delta, int nframes, double freq);
int class_calibration_rotation (GQueue *feature_list, config_t *config, GQueue *hist, lcm_t *lcm);
void class_calibration_find_start_end (GQueue *feature_list, int *, int *, int *);
int class_calibration (GQueue *feature_list, config_t *config, int step);
void class_imu_validation (GQueue *feature_list, GQueue *upward_image_queue, GQueue *pose_queue, config_t *config, char *filename);
void lr3_calib_to_matrix ();
int applanix_delta (GQueue *data, int64_t utime1, int64_t utime2, double *delta_deg);
double class_psi_distance (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2, lcm_t *lcm);
double class_phi_distance (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2);
int class_read_calib (config_t *cfg);
int class_write_calib (config_t *cfg, const char *filename);
char *class_next_calib_filename ();
char *class_next_map_filename ();

motion_classifier_t *motion_classifier_init (int size);
void motion_classifier_update (motion_classifier_t *m, double *prob);
void motion_classifier_write_top_two_motions (motion_classifier_t *mc, const char *filename, int64_t utime);
void motion_classifier_extract_top_two_motions (motion_classifier_t *mc, int *motion1, int *motion2);
void motion_classifier_print (motion_classifier_t *mc);
void motion_classifier_update_history (motion_classifier_t *mc, int motion1, int motion2);
int motion_classifier_histogram_voting (motion_classifier_t *mc, int startindex);
void motion_classifier_clear_history (motion_classifier_t *mc, int startindex);
void class_features_signature (navlcm_feature_list_t *f);
double class_guidance_derivative_sliding_window (double angle, int windowsize);

classifier_hist_t *classifier_hist_create_with_data (int nbins, double range_min, double range_max);
void classifier_hist_insert (classifier_hist_t *c, double val);
void classifier_destroy (gpointer data);
void classifier_hist_write_to_file (classifier_hist_t *c, char *filename);
navlcm_dictionary_t *classifier_hist_to_dictionary (classifier_hist_t *c);

#endif
