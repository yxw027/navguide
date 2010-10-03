#ifndef _BAGS_H__
#define _BAGS_H__

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// from glib
#include <glib.h>

// from navlcm
#include <lcmtypes/navlcm_feature_t.h>
#include <lcmtypes/navlcm_feature_list_t.h>
#include <lcmtypes/navlcm_features_param_t.h>

// from common
#include <common/mathutil.h>
#include <common/fileio.h>
#include <common/dbg.h>
#include <common/mkl_math.h>

#include "corrmat.h"

#define BAGS_MAX_CHILDREN 500
#define BAGS_KMEAN_N_CHILDREN 10
#define BAGS_MAX_N_SEARCH 5
#define BAGS_WORD_RADIUS .30

struct bag_t {
    float *cc;                // bag centroid
    int cc_size;
    int n;                  // # of features in bag (= size of the queue, we keep a copy for efficiency)
    GQueue *ind;             // list of indices (e.g. utime, or gate ID)
};

struct tree_node_t {
    float *cc; // centroid
    int cc_size;           // data size
    GQueue *bags;         // a queue of bags
                        // for a leaf node, size = g_queue_get_length (bags)
    int id;
};

int bag_t_init (bag_t *b, navlcm_feature_t *ft, int id);
int bag_t_insert_feature (bag_t *b, navlcm_feature_t *ft, int id);
int bag_t_search_index (bag_t *b, int index);
void tree_t_print (GNode *node);

int bag_tree_t_insert_feature_list (GQueue *bags, GNode *node, navlcm_feature_list_t *fs, unsigned int N, float bag_radius, int id);
GNode *tree_init_from_void (int size);

void node_add_bag (GNode *node, bag_t *bag) ;
void tree_t_node_update_full (GNode *node);
int tree_t_count_bags (GNode *node);
GQueue * tree_t_collect_bags (GNode *node);

GNode *tree_init_from_bags (GQueue *bags);
GNode *tree_init_from_bag (bag_t *bag);
void tree_insert_bag (GNode *node, bag_t *bag);
void tree_print_info (GNode *node);
void tree_update_bag (GNode *root, bag_t *bag, GNode *child);
gboolean tree_child_contains_bag (GNode *node, bag_t *bag);
void tree_unit_testing (int num_features, unsigned int N, float bag_radius);
float node_distance_d (GNode *node, float *data);
float node_distance (GNode *node, unsigned char *data);
void bag_tree_t_search_feature (GQueue *bags, GNode *node, navlcm_feature_t *ft, unsigned int N, float bag_radius, bag_t **bbag, float *mindist);

// naive implementation: is actually faster than optimized implementation
// thanks to MKL...
void bags_init (GQueue *bags, navlcm_feature_list_t *features, int id);
void bags_destroy (GQueue *bags);
void bags_append (GQueue *bags, GQueue *features, int id);
void bags_update (GQueue *bags, int id);
void bags_naive_search (GQueue *bags, navlcm_feature_list_t *features, double search_radius, GQueue *qfeatures, GQueue *qbags, double *usecs, gboolean use_mkl);
double *bags_naive_vote (GQueue *bags, int size);
void bags_performance_testing (int mode, int nsets, int nfeatures, unsigned int N, char *filename);

#endif

