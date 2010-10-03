#ifndef _GATES_H__
#define _GATES_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

/* from lcm*/
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_feature_list_t.h>
#include <lcmtypes/navlcm_feature_match_set_t.h>
#include <lcmtypes/navlcm_dictionary_t.h>

/* from common */
#include <common/lcm_util.h>
#include <common/fileio.h>
#include <common/timestamp.h>

/* from tracker */
#include "matcher.h"

/* from jpegcodec */
#include <jpegcodec/ppmload.h>
#include <jpegcodec/jpegload.h>
#include <jpegcodec/ppmload.h>

#include <guidance/tracker.h>
#include <guidance/classifier.h>

// from here
#include <guidance/bags.h>
#include <guidance/corrmat.h>

/* from graphviz */
#include <gvc.h>

typedef struct {
    int    uid;
    GQueue *img;        // a queue of botlcm_image_t
    GQueue *neighbors;  // a queue of gate_t
    GQueue *features;   // a queue of navlcm_feature_list_t
    
    botlcm_pose_t pose;
    navlcm_gps_to_local_t gps_to_local;
    botlcm_image_t up_img;

    char *label;
    double pdf0;
    double pdf1;

    gboolean checkpoint;

} gate_t;

gate_t *gate_new (int nsensors, int uid, gboolean checkpoint);
void gate_insert_feature (gate_t *g, navlcm_feature_t *f, int width, int height, int64_t utime, int dist_mode);
void gate_insert_features (gate_t *g, navlcm_feature_list_t *fs);
navlcm_feature_list_t* gate_features (gate_t *g);
int gate_orientation (gate_t *g, navlcm_feature_list_t *fs, config_t *config, 
        double *angle_rad, lcm_t *lcm);

gate_t* find_gate_from_id (GQueue *gates, int id);
void gate_update_image (gate_t *g, botlcm_image_t *img, int mode);
void gate_update_up_image (gate_t *g, gpointer data);
void gate_save_image_to_file (gate_t *g, const char *dirname);
void gate_insert_neighbor (gate_t *g, int n_id);
double gate_similarity (gate_t *g, navlcm_feature_list_t *fs);
int64_t gate_utime (gate_t *g);
gboolean gate_is_neighbor (gate_t *g1, gate_t *g2);
void gate_set_pose (gate_t *gate, botlcm_pose_t *p);
void gate_set_gps_to_local (gate_t *gate, navlcm_gps_to_local_t *g);
gate_t *gate_find_by_uid (GQueue *gates, int uid);
void gates_merge (GQueue *gates, int id1, int id2);
void gates_merge (GQueue *gates, GQueue *components);
void gates_sanity_check (GQueue *gates);
void gates_renumber (GQueue *gates);
void gates_assign_linear_neighbors (GQueue *gates);
void gates_print (GQueue *gates);
void gates_remove_singletons (GQueue *gates);
void gate_clear_neighbors (gate_t *g);
GQueue *gates_create_from_component_list (GQueue *gates, float *mat, int size, int maxsize, int *alias);
void gates_cleanup (GQueue *gates);
void gate_list_neighbors (gate_t *g, GQueue *gates, int radius, GQueue *neighbors);
void gates_save_layout (GQueue *gates, const char *mode, const char *format, const char *filename);
void gate_save_image_tile_to_file (gate_t *g, const char *filename);
void gates_save_image_tile_to_file (GQueue *gates);
void gates_process_similarity_matrix (const char *filename, GQueue *gates, gboolean smooth, char *data_dir, gboolean demo, lcm_t *lcm);
void gates_compute_similarity_matrix (GQueue *gates);
double* gates_loop_closure_update (GQueue *voctree, double *corrmat, int *corrmat_size, navlcm_feature_list_t *features, int id, double bag_word_radius);

void gates_publish_labels (GQueue *gates, lcm_t *lcm, const char *channel);
int gate_set_label (gate_t *g, const char *label);
int gates_closest_with_label (GQueue *gates, gate_t *g);
int gates_min_id (GQueue *gates);
int gates_max_id (GQueue *gates);

#endif

