#ifndef _CLASS_SUBDIV_H__
#define _CLASS_SUBDIV_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include <common/dbg.h>
#include <common/mathutil.h>
#include <common/fileio.h>
#include <common/lcm_util.h>

#include <lcmtypes/navlcm_class_data_t.h>
#include <lcmtypes/navlcm_feature_match_set_t.h>
#include <lcmtypes/navlcm_dead_reckon_t.h>

#define CLASS_ROT_MODE 0
#define CLASS_TRA_MODE 1
#define CLASS_MIN_VOTES 100

#define CLASS_TMP_DIR CONFIG_DIR"/class.tmp"
#define HIST_SIZE 200

struct odometry_t { double dist; double angle_rad;};
struct cell_t { int n; double *vals;};

double class_get_data (navlcm_class_data_t *data, int s, int id1, int id2);
void class_set_data (navlcm_class_data_t *data, int s, int id1, int id2, double val);
void class_inc_data (navlcm_class_data_t *data, int s, int id1, int id2, double val);
int class_get_data_n (navlcm_class_data_t *data, int s, int id1, int id2);
void class_set_data_n (navlcm_class_data_t *data, int s, int id1, int id2, int val);
void class_inc_data_n (navlcm_class_data_t *data, int s, int id1, int id2, int val);
cell_t*** classifier_reset (navlcm_class_data_t *data, cell_t ***cells);
void classifier_destroy (navlcm_class_data_t *data);
navlcm_class_data_t *classifier_init_tables ();
navlcm_class_data_t *classifier_init_tables (int nsensors, int nbuckets);
int classifier_get_index_from_table (int index, int nsensors, int *sid1, 
                                     int *sid2, navlcm_class_data_t *data);
int classifier_get_table_index (int sid1, int sid2, int *transpose,
                                navlcm_class_data_t *data);
int classifier_image_coord_to_bucket_id (float col, float row, int width, 
                                         int height, int nbuckets);
void classifier_bucket_id_to_image_coord (int id, int width, int height, int nbuckets, double *col, double *row);
void classifier_stat (navlcm_class_data_t *data, double *mean, double *stdev);
void classifier_cleanup (navlcm_class_data_t *data, int minvotes);
void classifier_force_diagonal (navlcm_class_data_t *data);
void dyn_fill_classifier (navlcm_class_data_t *data, int sensor1, int sensor2);
void dyn_fill_classifier (navlcm_class_data_t *data);
void set_table (navlcm_class_data_t *data, int index, int id1, int id2,
                double value, int nval);
void update_table (navlcm_class_data_t *data, int index, int id1, int id2, 
                   double value, int mode);
cell_t*** classifier_add_matches (navlcm_class_data_t *data, int width, 
                                  int height, cell_t ***cells,
                                  navlcm_feature_match_set_t *ma, 
                                  navlcm_dead_reckon_t dr, int mode);
void classifier_update_tables (navlcm_class_data_t *data, 
                               cell_t ***cells, int mode);
void classifier_save_to_file (const char *filename, 
                              cell_t ***cells,
                              navlcm_class_data_t *data);
void classifier_save_to_ascii_file (navlcm_class_data_t *data, 
                                    const char *basename);
navlcm_class_data_t *classifier_load_from_file (const char *filename,
                                                  cell_t ***cells);
gboolean classifier_query (navlcm_class_data_t *data,
                           int width, int height, int sid1, int sid2,
                           double col1, double col2,
                           double row1, double row2,
                           double *val, int *nval);
navlcm_dead_reckon_t *classifier_query (navlcm_class_data_t *data, 
                                          int width, int height,
                                          navlcm_feature_match_set_t *ma, 
                                          int *n, int mode, int count);
navlcm_dead_reckon_t *classifier_normalize_dead_reckon_dist (navlcm_dead_reckon_t *drs, int n, 
                                                               int *new_n, int nsensors);
double classifier_ransac (double *data, int n, int mode, int nruns, int ninliers, double *error);
navlcm_dead_reckon_t 
classifier_extract_single_dead_reckon (navlcm_dead_reckon_t *drs, int n, 
                                       int mark, double *error, int mode);
navlcm_feature_match_set_t *classifier_filter_matches (
                 navlcm_class_data_t *data, int width, int height, 
                 navlcm_feature_match_set_t *matches,
                 double angle_thresh_deg, gboolean strict);
double dead_reckon_list_to_odometry (navlcm_dead_reckon_t *drs, int ndrs, int nsensors);
void save_odometry_to_file (odometry_t *odometry, int num_odometry, const char *filename);
odometry_t *read_odometry_from_file (int *num_odometry, const char *filename);
cell_t*** cell_t_init (navlcm_class_data_t *data);
void cell_t_destroy (navlcm_class_data_t *data, 
                     cell_t ***cells);
cell_t*** cell_t_insert (cell_t ***cells, int n, int i, int j, double val);
void classifier_insert_prob_hash (GHashTable *hash, 
                    navlcm_feature_match_set_t *matches,
                    int width, int height, int nbuckets);
int classifier_sum_values_prob_hash (GHashTable *hash);
int classifier_sum_values_prob_hash (GHashTable *hash, int sid, int index);
gboolean classifier_test_prob_hash (GHashTable *hash, 
                                double col1, double row1,
        int sid1, double col2, double row2, int sid2, int width, int height, 
        int nbuckets, int thresh);
navlcm_feature_match_set_t* classifier_filter_prob_hash (GHashTable *hash, 
        navlcm_feature_match_set_t* matches, int width, int height, int nbuckets);
void classifier_save_to_file_prob_hash (GHashTable *hash, const char *filename);
void classifier_read_from_file_prob_hash (GHashTable *hash, const char *filename);
GHashTable *classifier_normalize_prob_hash (GHashTable *hash);

cell_t*** classifier_add_matches (navlcm_class_data_t *data, 
                                  int width, int height, 
                                  cell_t ***cells,
                                  navlcm_feature_match_set_t *ma, 
                                  navlcm_dead_reckon_t dr, int mode);
void classifier_save_to_file (const char *filename, 
                              cell_t ***cells,
                              navlcm_class_data_t *data);

navlcm_class_data_t *classifier_load_from_file (const char *filename,
                                                  cell_t ***cells);
cell_t ***classifier_reset (navlcm_class_data_t *data, cell_t ***cells);
void classifier_update_tables (navlcm_class_data_t *data, 
                               cell_t ***cells, int mode);

cell_t*** cell_t_init (navlcm_class_data_t *data);
void cell_t_destroy (navlcm_class_data_t *data, 
                     cell_t ***values);
cell_t*** cell_t_insert (cell_t ***cells, int n, int i, int j, double val);
void save_odometry_to_file (odometry_t *travel, int num_travel, 
                            const char *filename);
odometry_t *read_odometry_from_file (int *num_travel, const char *filename);

#endif

