#ifndef _CLASS_DIJKSTRA_H__
#define _CLASS_DIJKSTRA_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

/* from lcm*/
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_feature_list_t.h>
#include <lcmtypes/navlcm_feature_match_set_t.h>
#include <lcmtypes/navlcm_gps_to_local_t.h>
#include <lcmtypes/navlcm_dictionary_t.h>
#include <lcmtypes/navlcm_ui_map_t.h>

#include <bot/bot_core.h>

/* from common */
#include <common/lcm_util.h>
#include <common/fileio.h>
#include <common/timestamp.h>

/* from tracker */
#include "matcher.h"
#include "corrmat.h"

/* from jpegcodec */
#include <jpegcodec/ppmload.h>
#include <jpegcodec/jpegload.h>
#include <jpegcodec/ppmload.h>

#include <guidance/tracker.h>
#include <guidance/classifier.h>

/* from graphviz */
#include <gvc.h>

#include "util.h"

/* a structure for the dijkstra's algorithm
 */
typedef struct { GQueue *nodes; GQueue *edges; GHashTable *alias; } dijk_graph_t;

typedef struct { 
   
    // node data
    char *label;
    gboolean checkpoint;
    int64_t utime;
    int uid;
    double pdf0;
    double pdf1;
    GQueue *edges; 

    // for dijkstra's algorithm
    int64_t timestamp; 
    int dist; 
    gpointer previous;

} dijk_node_t;

typedef struct { 
    // edge data
    navlcm_feature_list_t *features;
    GQueue *img;
    botlcm_image_t *up_img;
    botlcm_pose_t *pose;
    navlcm_gps_to_local_t *gps_to_local;
    int motion_type;

    dijk_node_t *start;
    dijk_node_t *end;
    gboolean reverse;

    int64_t timestamp;

} dijk_edge_t;


dijk_edge_t *dijk_edge_new (navlcm_feature_list_t *f, GQueue *img, botlcm_image_t *up_img, botlcm_pose_t *pose, navlcm_gps_to_local_t *gps, gboolean reverse, dijk_node_t *start, dijk_node_t *end, int motion_type);
dijk_node_t *dijk_node_new (int uid, gboolean checkpoint, int64_t utime);
dijk_graph_t *dijk_graph_new ();

void dijk_graph_insert_nodes (dijk_graph_t *dg, GQueue *nodes);
void dijk_graph_insert_node (dijk_graph_t *dg, dijk_node_t *n);
void dijk_graph_insert_edge (dijk_graph_t *dg, dijk_edge_t *n);
dijk_edge_t *dijk_graph_latest_edge (dijk_graph_t *dg);

void dijk_graph_add_new_node (dijk_graph_t *g, navlcm_feature_list_t *f, GQueue *img, botlcm_image_t *up_img, botlcm_pose_t *pose, navlcm_gps_to_local_t *gps, gboolean checkpoint, dijk_edge_t *current_edge, int motion_type);

void dijk_graph_init (dijk_graph_t *dg);
void dijk_graph_destroy (dijk_graph_t *dg);
void dijk_node_destroy (dijk_node_t *nd);
void dijk_edge_destroy (dijk_edge_t *nd);
void dijk_edge_print (dijk_edge_t *e);
void dijk_graph_print (dijk_graph_t *dg);

dijk_graph_t *dijk_graph_copy (dijk_graph_t *dg);

dijk_edge_t *dijk_graph_find_edge_by_id (dijk_graph_t *dg, int id0, int id1);
dijk_node_t *dijk_graph_find_node_by_id (dijk_graph_t *dg, int id);
dijk_edge_t *dijk_node_find_edge (dijk_node_t *n1, dijk_node_t *n2);
GQueue *dijk_node_neighbors (dijk_node_t *nd);
dijk_edge_t *dijk_find_edge (GQueue *edges, dijk_node_t *n);
dijk_edge_t *dijk_graph_find_edge_by_id (dijk_graph_t *dg, int id0, int id1);
void dijk_graph_remove_edge_by_id (dijk_graph_t *dg, int id1, int id2);

dijk_edge_t *dijk_get_nth_edge (dijk_node_t *n, int p);
navlcm_feature_list_t *dijk_node_get_nth_features (dijk_node_t *n, int p);
void dijk_node_set_label (dijk_node_t *n, const char *txt);
int dijk_graph_max_node_id (dijk_graph_t *dg);
double dijk_edge_time_length (dijk_edge_t *e);

gboolean dijk_node_has_neighbor (dijk_node_t *n1, dijk_node_t *n2);
void dijk_tree_print (GNode *node);
GNode* dijk_to_tree (dijk_node_t *nd, int radius) ;
GNode* dijk_to_dual_tree (dijk_edge_t *ed, int radius) ;
GQueue *dijk_find_shortest_path (dijk_graph_t *dg, dijk_node_t *src, dijk_node_t *dst);
void dijk_unit_testing ();
void dijk_graph_read (dijk_graph_t *g, FILE *fp);
void dijk_graph_write (dijk_graph_t *g, FILE *fp);
void dijk_graph_read_from_file (dijk_graph_t *g, const char *filename);
void dijk_graph_write_to_file (dijk_graph_t *g, const char *filename);
void dijk_apply_components (dijk_graph_t *dg, GQueue *components);

void dijk_save_images_to_file (dijk_graph_t *dg, const char *dirname, gboolean overwrite);
void dijk_graph_layout_to_file (dijk_graph_t *dg, const char *mode, const char *format, const char *filename, int max_nodeid);
void dijk_graph_remove_triplets (dijk_graph_t *dg);
int dijk_graph_n_nodes (dijk_graph_t *dg);
void dijk_graph_test_mission (dijk_graph_t *dg, GQueue *mission);
void dijk_graph_remove_node_by_id (dijk_graph_t *dg, int id);
void dijk_print_component (dijk_graph_t *dg, component_t *c, const char *corrmat_filename, const char *filename);
void dijk_graph_print_node_utimes (dijk_graph_t *dg, const char *filename);

void dijk_graph_merge_nodes_by_id (dijk_graph_t *dg, int id1, int id2);
void dijk_graph_merge_nodes (dijk_graph_t *dg, dijk_node_t *n1, dijk_node_t *n2);
void dijk_graph_enforce_type_symmetry (dijk_graph_t *dg);


navlcm_ui_map_t *dijk_graph_to_ui_map_basic (dijk_graph_t *dg, const char *mode);
navlcm_ui_map_t *dijk_graph_to_ui_map (dijk_graph_t *dg, const char *mode);
void dijk_graph_ground_truth_localization (dijk_graph_t *dg, botlcm_pose_t *pose, GQueue *fix_poses, dijk_node_t *node, int num_features, const char *filename);
void dijk_graph_fix_poses (dijk_graph_t *dg, GQueue *poses);
void dijk_integrate_time_distance (GQueue *path, double *time_secs, double *distance_m);
int dijk_find_future_direction_in_path (GQueue *path, int mindepth, int maxdepth, int *motion_type, int size);


#endif

