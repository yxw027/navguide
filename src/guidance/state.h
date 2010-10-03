#ifndef _MISSION_STATE_H__
#define _MISSION_STATE_H__

#include <stdlib.h>
#include <math.h>

#include <common/dbg.h>
#include <dijkstra.h>
#include <classifier.h>

void state_transition_update (dijk_graph_t *dg, int radius, double state_sigma);
void state_observation_update (dijk_graph_t *dg, dijk_edge_t *e, int radius, navlcm_feature_list_t *f, GQueue *path, double *variance);
void state_init (dijk_graph_t *dg, dijk_node_t *n);
void state_print (dijk_graph_t *dg, dijk_node_t *cg);
void state_print_to_file (dijk_graph_t *dg, const char *filename);
void state_find_maximum (dijk_graph_t *dg, dijk_node_t **og);
void state_update_class_param (navlcm_class_param_t *state, dijk_graph_t *dg);
double state_sum (dijk_graph_t *dg);

#endif
