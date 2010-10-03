#ifndef _CLASS_CORRMAT_H__
#define _CLASS_CORRMAT_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <glib.h>

#include <common/dbg.h>
#include <common/mathutil.h>

#include <lcmtypes/navlcm_dictionary_t.h>

#define CORRMAT_SET(m, n, i, j, val) ((m)[(i)*(n)+(j)] = (val))
#define CORRMAT_GET(m, n, i, j) ((m)[(i)*(n)+(j)])


struct component_t { GQueue *pt; double score; gboolean reverse;};

double *corrmat_init (int n, double val);
void corrmat_print (double *m, int n);
double *corrmat_resize_once (double *m, int *n, double val);
void corrmat_int_write (int *corrmat, int n, const char *filename);
void corrmat_write (double *corrmat, int n, const char *filename) ;
void corrmat_smoothen (double *corrmat, int n);
void corrmat_compute_alignment_matrix (double *corrmat, int n, double thresh, double delta, gboolean rev, double **hout, int **hindout);
double corrmat_local_max (double *h, int n, int radius, int ri, int rj);
void corrmat_reverse (double *mat, int n);
void corrmat_int_reverse (int *mat, int n);
void component_t_print (component_t *c);
void component_t_free (component_t *c);
component_t *component_t_new ();

void corrmat_find_component_list (double * hmat, int * hmatind, double * hmatrev, int* hmatindrev, int n, double alignment_threshold, double alignment_tail_thresh, int min_seq_length, int alignment_min_diag_distance, double alignment_max_slope_error, int alignment_search_radius, int alignment_min_node_id, GQueue *components);

void corrmat_cleanup_component (component_t *c);
void corrmat_cleanup_component_list (GQueue *components);
int corrmat_find_component (double * hmat, int * hmatind, double * hmatrev, int* hmatindrev, int n, double minthresh, double maxthresh, component_t *comp, gboolean *reverse);
gboolean component_t_valid (component_t *c, int n, int radius);
int component_t_cmp (gconstpointer a, gconstpointer b, gpointer data);
int corrmat_find_local_max (double *h, int n, double minval, double maxval, int radius, int *maxi, int *maxj);
void corrmat_convolve (double *m, int size, double *kern, int ksize);
void corrmat_threshold (double *h, int n, double s);
void corrmat_shift (double *h, int n, double s);
double corrmat_min (double *h, int n);
double corrmat_max (double *h, int n);
int *corrmat_copy (int *mat, int n);
double *corrmat_copy (double *mat, int n);
double *corrmat_read (const char *filename, int *n);

navlcm_dictionary_t * corrmat_to_dictionary (double *mat, int size);
double *dictionary_to_corrmat (const navlcm_dictionary_t *dict, int *size);

#endif

