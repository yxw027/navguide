#ifndef _MATHUTIL_H
#define _MATHUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <glib.h>

#ifndef PI
#define PI 3.14159265358979323846264338
#endif

#ifndef ABS
#define ABS(x)    (((x) > 0) ? (x) : (-(x)))
#define MAX(x,y)  (((x) > (y)) ? (x) : (y))
#define MIN(x,y)  (((x) < (y)) ? (x) : (y))
#endif

#ifndef NAVGUIDE_EPS
#define NAVGUIDE_EPS 1E-6
#endif

double mod2pi(double vin);
double mod2pi_ref(double ref, double vin);

#define to_radians(x) ( (x) * (M_PI / 180.0 ))
#define to_degrees(x) ( (x) * (180.0 / M_PI ))

static inline double sq(double v)
{
  return v*v;
}

static inline double sgn(double v)
{
  return (v>=0) ? 1 : -1;
}

void quat_angle_unit_testing (int n);
double quat_angle (double q1[4], double q2[4]);
double quat_angle_2 (double q1[4], double q2[4]);
double clip_value (double val, double min, double max, double eps);
double diff_angle_plus_minus_180 (double a1, double a2);
double diff_angle_plus_minus_pi (double a1, double a2);
int math_square_distance (unsigned char *a1, unsigned char *a2, int n);
int sum (int *val, int n);
double math_median_double (double *val, int n);
unsigned char math_median (unsigned char *val, int n);
double math_quick_select(double *vals, int n);
unsigned char* math_smooth (unsigned char *data, int size, int window);
double *math_smooth_2pi (double *data, int size, int window);
double* math_smooth_double (double *data, int size, int window);
int *math_smooth_int (int *data, int size, int window);
int *math_filter_majority (int *data, int size, int window);

int math_max_index_double (double *data, int n);

int intersect (double x1, double y1, double x2, double y2, 
               double x3, double y3, double x4, double y4,
               double err, double *x, double *y, double *l1, double *l2);

double math_gaussian (double x, double mu, double sigma);
double math_square_distance_double (double x1, double y1, double x2, double y2);
void histogram_write_to_file (int *hist, int size, const char *filename);
void histogram_write_to_file_full (int *hist, int size, const char *filename, double minval, double maxval);
void histogram_write_to_file_double (double *hist, int size, const char *filename);
void histogram_write_to_file_double_full (double *hist, int size, const char *filename, double minval, double maxval);
int *histogram_build (double *data, int ndata, int size, double *, double *);
void math_normalize (double *data, int ndata);
void math_normalize_float (float *data, int ndata);
void histogram_min_max (double *data, int ndata, double *min, double *max);
int histogram_find_orientation (unsigned char *data, int size);

double histogram_variance (double *data, int ndata, double mean);
double histogram_mean (double *data, int ndata);
int histogram_peak (int *data, int ndata, int *peak);
void histogram_min_threshold (double *data, int ndata, double thresh);
int histogram_min (double *data, int ndata, double *val);
int histogram_max (double *data, int ndata, double *val);

int math_round (double a);
double math_mean_char (unsigned char *data, int n);
double math_mean_double (double *data, int n);


void quat_rot_axis (double *ax, double angle, double *p, double *p2);
int math_line_fit (double *pts, int n, double *a, double *b, double *theta);
int rand_int (int min, int max);
int *math_random_indices (int n, int min, int max);
int math_find_index (int *indices, int n, int index);


inline 
int math_square_distance (unsigned char *a1, unsigned char *a2, int n)
{
    int d = 0;
    for (int i=0;i<n;i++) 
        d += (a1[i]-a2[i])*(a1[i]-a2[i]);
    
    return d;
}

struct pair_t { int key; double val; };
struct pair_int_t { int key; int val; };

double math_stdev (double *data, int size);

gint pair_comp_key (gconstpointer d1, gconstpointer d2, gpointer data);
gint pair_comp_val (gconstpointer d1, gconstpointer d2, gpointer data);
gint pair_int_comp_key (gconstpointer d1, gconstpointer d2, gpointer data);
gint pair_int_comp_val (gconstpointer d1, gconstpointer d2, gpointer data);

// vectors
//
double *vect_read (FILE *fp, int *nvals);
void   vect_write (FILE *fp, double *vals, int nval);
double vect_variance (double *data, int ndata, double mean);
double vect_stdev (double *data, int ndata, double mean);
double vect_mean_double (double *data, int ndata);
unsigned char vect_mean_char (unsigned char *data, int ndata);
double vect_mean_sincos (double *data, int ndata);
double vect_weighted_mean_sincos (double *data, double *weight, int ndata);
double vect_variance_sincos (double *data, int ndata, double mean);
double vect_stdev_sincos (double *data, int ndata, double mean);

float vect_sqdist_char (float *d1, unsigned char *d2, int size);
double vect_sqdist_double (double *d1, unsigned char *d2, int size);
double vect_sqdist_double_double (double *d1, double *d2, int size);
float vect_sqdist_float (float *d1, float *d2, int size);

    float vect_dot_float (float *d1, float *f2, int size);
double vect_dot_double (double *d1, double *d2, int size);
void vect_norm (double *a, int n);
double vect_length_double (double *a, int n);
float vect_length_float (float *a, int n);
void vect_normalize (double *data, int n, double val);

double vect_dist_ncc (unsigned char *d1, unsigned char *d2, int size);
double vect_dist_min_ncc (double *d1, double *d2, int size, int max_col, int max_row);
double vect_dist_ncc_offset (double *d1, double *d2, int size, int width, int offset_col, int offset_row);


int *vect_append (int *data, int *size, int val);
int vect_search_binary (int *data, int size, int val);
int *vect_remove_binary (int *data, int *size, int val);

struct quartet { int a,b,c,d; };
typedef struct quartet quartet_t;

quartet_t* quartet_new (int a, int b, int c, int d);
guint m_quartet_hash (gconstpointer key);
gboolean m_quartet_equal (gconstpointer v1, gconstpointer v2);
gpointer m_quartetdup (const quartet_t *q);
void m_quartet_free (quartet_t *q);
void g_str_free (char *q);
double math_overlap (double a1, double a2, double b1, double b2);
gboolean math_project_2d (double x, double y, double x1, double y1, double x2, double y2, 
        double *lambda, double *xo, double *yo);
gboolean math_project_2d_segment (double x, double y, double x1, double y1, double x2, double y2,
                                  double *xo, double *yo, double *dist);
void math_cross_product (double *q, double *r, double *o);

int * g_intdup(gint value);
float *g_floatdup (float value);
gdouble *g_doubledup (gdouble value);
gboolean math_dist_2d (double x, double y, double a, double b, double c, double d, double *oxh, double *oyh, double *res);

// camera calibration
//
void calib_distort_1st_order (double *x, double *y, double k);
void calib_undistort_1st_order (double *x, double *y, double k);
void calib_undistort (double *x, double *y, double *k);
void calib_normalize (double *x, double *y, double *fc, double *cc);
void calib_denormalize (double *x, double *y, double *fc, double *cc);

float **math_init_mat_float (int m, int n, float val);
int **math_init_mat_int (int m, int n, int val);

double *math_3d_array_alloc_double (int p, int q, int r);
double math_3d_array_get_double (double *a, int p, int q, int r, int i, int j, int k);
void math_3d_array_set_double (double *a, int p, int q, int r, int i, int j, int k, double val);
int *math_3d_array_alloc_int (int p, int q, int r);
int math_3d_array_get_int (int *a, int p, int q, int r, int i, int j, int k);
void math_3d_array_set_int (int *a, int p, int q, int r, int i, int j, int k, int val);

void secs_to_hms (int isecs, int *hours, int *mins, int *secs);


#endif
