#ifndef _GUIDANCE_LOOP_H__
#define _GUIDANCE_LOOP_H__

#include "dijkstra.h"
#include "bags.h"
#include <common/mathutil.h>

/* From OpenCV */
#include <opencv/cv.h>
#include <opencv/highgui.h>

double* loop_update_full (GQueue *voctree, double *corrmat, int *corrmat_size, navlcm_feature_list_t *features, int id, double bag_word_radius);
component_t* loop_index_to_component (int x0, int x1, int y0, int y1, gboolean reverse, int min_length);
void loop_line_to_component (CvPoint *l, GQueue *components, gboolean reverse, int min_length);
GQueue* loop_extract_components_from_correlation_matrix (double *corrmat, int size, gboolean smooth, double canny_thresh, 
        double hough_thresh, int min_seq_length, double correlation_diag_size, double correlation_threshold, 
        double alignment_penalty, double alignment_threshold, double alignment_tail_thresh, 
        int alignment_min_diag_distance, double alignment_max_slope_error, int alignment_search_radius, int alignment_min_node_id);
void loop_matrix_components_to_image (double *corrmat, int size, GQueue *components);


#endif

