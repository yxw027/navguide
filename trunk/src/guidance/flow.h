#ifndef _guidance_flow_h__
#define _guidance_flow_h__

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* From OpenCV */
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <glib.h>
#include <common/dbg.h>
#include <common/mathutil.h>
#include <lcmtypes/navlcm_class_param_t.h>
#include <lcmtypes/navlcm_flow_t.h>

#include <bot/bot_core.h>
#include <GL/gl.h>

typedef struct {
    IplImage *flow_grey;
    IplImage *flow_prev_grey;
    IplImage *flow_pyramid;
    IplImage *flow_prev_pyramid;
    IplImage *flow_swap_temp;
    CvPoint2D32f *flow_points[2], *flow_swap_points;
    char *flow_status;
    int flow_flags;
    int flow_need_to_init;
    int flow_count;
    int64_t utime0;
    int64_t utime1;
    int width;
    int height;
} flow_t;

typedef struct {
    double *flowx;
    double *flowy;
    int *nflow;
    int nbins;
} flow_field_t;

typedef struct {
    flow_field_t *fields;
    int nfields;
    char *name;
} flow_field_set_t;


flow_t *flow_init ();
void flow_t_destroy (flow_t *f);
void flow_reverse (flow_t *f);
void flow_good_features_to_track (IplImage *img, CvPoint2D32f *points, int *count);
flow_t *flow_compute (unsigned char *data1, unsigned char *data2, int width, int height, int64_t utime1, int64_t utime2, double scale, double *success_rate);
IplImage* flow_to_image (flow_t *f);
void flow_draw (flow_t *f, char *fname);

void flow_field_init (flow_field_t *fc, int nbins);
void flow_set_draw (flow_t **f, int num, char *fname);
void FLOW_FIELD_SET (flow_field_t *fc, double x, double y, double fx, double fy, int n);
void FLOW_FIELD_GET_WITH_INDEX (flow_field_t *fc, int idx, int idy, double *fx, double *fy, int *n);
void FLOW_FIELD_GET (flow_field_t *fc, double x, double y, double *fx, double *fy, int *n);
void FLOW_FIELD_UPDATE (flow_field_t *fc, double x, double y, double fx, double fy);
void FLOW_FIELD_MAX_N (flow_field_t *fc, int *maxn);
void flow_field_reset (flow_field_t *fc);
void flow_field_to_image (flow_field_t *fc, IplImage *imf, IplImage *imn);
void flow_field_draw (flow_field_t *fc, int width, int height, char *fname1, char *fname2);
double flow_field_vec_length (flow_field_t *f);
void flow_field_set_draw (flow_field_set_t *fc, int width, int height, char *fname);
int flow_field_update_from_flow (flow_field_t *fc, flow_t *f);
int flow_field_write_to_file (flow_field_t *fc, FILE *fp);
int flow_field_read_from_file (flow_field_t *fc, FILE *fp);
flow_field_set_t* flow_field_set_init ();
flow_field_set_t* flow_field_set_init_with_data (int nfields, int nbins, char *name);
int flow_field_set_write_to_file (flow_field_set_t *fc, FILE *fp);
int flow_field_set_read_from_file (flow_field_set_t *fc, FILE *fp);
int flow_field_set_series_read_from_file (GQueue *flow_fields, char **filenames, char **names, int nnames, int nsensors, const char *dirname);
double flow_field_similarity (flow_field_t *fc1, flow_field_t *fc2);
double *flow_field_set_score_motions (GQueue *ref_fields, flow_field_set_t *fc);
void flow_process_scores (GQueue *ref_fields, double *scores, int *motion1, int *motion2);
navlcm_flow_t* flow_field_set_to_navlcm_flow (flow_field_set_t *f);

#endif

