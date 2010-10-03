#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include <bot/bot_core.h>

#include "camtrans.h"

#if 1
#define ERR(...) do { fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
                      fprintf(stderr, __VA_ARGS__); fflush(stderr); } while(0)
#else
#define ERR(...) 
#endif

#if 1
#define DBG(...) do { fprintf(stdout, __VA_ARGS__); fflush(stdout); } while(0)
#else
#define DBG(...) 
#endif

#define CAMERA_EPSILON 1e-10

// Function that produces values for the warp tables
typedef int (*warp_func_t)(const double, const double, const void*,
                           double*, double*);

// Structure for spherical distortion model
typedef struct {
    double w;   // Original image width
    double h;   // Original image height
    double cx;  // Center of distortion (normalized coords)
    double cy;
    double a;   // Spherical distortion parameter

    // Temporaries for faster computation
    double inv_w;
    double x_trans;
    double y_trans;
} SphericalDistortion;

// Construct spherical distortion object, initialized with parameters
static SphericalDistortion*
spherical_distortion_create (const double w, const double h,
                             const double cx, const double cy,
                             const double a)
{
    SphericalDistortion *dist =
        (SphericalDistortion*)calloc(1, sizeof(SphericalDistortion));
    assert (NULL != dist);

    dist->w = w;
    dist->h = h;
    dist->cx = cx;
    dist->cy = cy;
    dist->a = a*a*a*a;
    dist->inv_w = 1/w;
    dist->x_trans = 0.5 + cx;
    dist->y_trans = 0.5*h/w + cy;

    return dist;
}

// Destructor
static void
spherical_distortion_destroy (SphericalDistortion *dist)
{
  free(dist);
}

// Undistort according to spherical model
static int
spherical_undistort_func (const double x, const double y, const void *data,
                          double *ox, double *oy)
{
    SphericalDistortion *dist = (SphericalDistortion*)data;

// FIXME wtf?
//#warning REALLY BAD HACK BELOW TO FIX PROBLEM WITH UNDISTORT AND REALLY WIDE CAMERA
    double x_adjusted = x;
    if (x_adjusted < 70)
        x_adjusted = 70;

    *ox = x_adjusted * dist->inv_w - dist->x_trans;
    *oy = y * dist->inv_w - dist->y_trans;

    double r2 = *ox * *ox + *oy * *oy;
    double new_r2 = r2/(1 - r2*dist->a);

    double ratio;
    if (fabs (r2) < 1e-8)
        ratio = 0;
    else
        ratio = new_r2/r2;

    if (ratio >= 0) {
        ratio = sqrt (ratio);
        *ox = (*ox*ratio + dist->x_trans)*dist->w;
        *oy = (*oy*ratio + dist->y_trans)*dist->w;
    }
    else {
        *ox = *oy = -1e50;
        return -1;
    }

    return 0;
}

// Distort according to spherical model
static int
spherical_distort_func (const double x, const double y, const void *data,
                        double *ox, double *oy)
{
    SphericalDistortion *dist = (SphericalDistortion*)data;

    *ox = x * dist->inv_w - dist->x_trans;
    *oy = y * dist->inv_w - dist->y_trans;

    double r2 = *ox * *ox + *oy * *oy;
    double new_r2 = r2/(1 + r2*dist->a);

    double ratio;
    if (fabs (r2) < 1e-8)
        ratio = 0;
    else
        ratio = new_r2/r2;

    ratio = sqrt (ratio);
    *ox = (*ox*ratio + dist->x_trans)*dist->w;
    *oy = (*oy*ratio + dist->y_trans)*dist->w;

    return 0;
}

// Structure to hold camera data
struct _CamTrans {
    char *name;
    double width;             // Image width in pixels
    double height;            // Image height in pixels

    double fx;
    double fy;
    double cx;
    double cy;
    double skew;

    double matx[9];           // Camera projection matrix
    double inv_matx[9];       // Inverse camera projection matrix

    void *dist_data;          // Distortion/undistortion data
    warp_func_t undist_func;  // Undistortion function
    warp_func_t dist_func;    // Distortion function
};

static void camtrans_compute_matrices (CamTrans *self);

CamTrans*
camtrans_new_spherical (const char *name,
        double width, double height,
        double fx, double fy,
        double cx, double cy, double skew,
        const double distortion_cx, const double distortion_cy,
        double distortion_param)
{
    CamTrans *self = (CamTrans*)calloc(1, sizeof(CamTrans));
    assert (NULL != self);

    if(name) 
        self->name = strdup(name);
    self->width = width;
    self->height = height;

    self->fx = fx;
    self->fy = fy;
    self->cx = cx;
    self->cy = cy;
    self->skew = skew;

    // Compute porjection matrix representations
    //self->matx[0] = fx; self->matx[1] = skew; self->matx[2] = cx;
    //self->matx[3] = 0;  self->matx[4] = fy;   self->matx[5] = cy;
    //self->matx[6] = 0;  self->matx[7] = 0;    self->matx[8] = 1;
    //int status = bot_matrix_inverse_3x3d (self->matx, self->inv_matx);
    //if (0 != status) {
    //    ERR("Error: camera matrix is singular\n");
    //    goto fail;
    //}

    camtrans_compute_matrices (self);

    // Create distortion/undistortion objects
    self->dist_data = (void*)spherical_distortion_create (self->width, self->height,
                                                          distortion_cx, distortion_cy, 
                                                          distortion_param);
    
    self->undist_func = spherical_undistort_func;
    self->dist_func = spherical_distort_func;

    return self;
}

static void 
camtrans_compute_matrices (CamTrans *self)
{
    double pinhole[] = {
        self->fx, self->skew, self->cx,
        0,        self->fy,   self->cy,
        0,        0,          1 
    };
    memcpy(self->matx, pinhole, 9*sizeof(double));
    int status = bot_matrix_inverse_3x3d (self->matx, self->inv_matx);
    if (0 != status) {
        fprintf (stderr, "WARNING: camera matrix is singular (%s:%d)\n",
                 __FILE__, __LINE__);
    }
}


void
camtrans_destroy (CamTrans *self)
{
    if (NULL == self)
        return;

    if (NULL != self->dist_data)
        spherical_distortion_destroy ((SphericalDistortion*)self->dist_data);

    free(self->name);
    free(self);
}

const char *
camtrans_get_name(const CamTrans *self)
{
    return self->name;
}

double
camtrans_get_focal_length_x (const CamTrans *self)
{
    return self->fx;
}
double
camtrans_get_focal_length_y (const CamTrans *self)
{
    return self->fy;
}
double
camtrans_get_image_width (const CamTrans *self)
{
    return self->width;
}
double
camtrans_get_image_height (const CamTrans *self)
{
    return self->height;
}
double
camtrans_get_principal_x (const CamTrans *self)
{
    return self->cx;
}
double
camtrans_get_principal_y (const CamTrans *self)
{
    return self->cy;
}
double
camtrans_get_width (const CamTrans *self)
{
    return self->width;
}
double
camtrans_get_height (const CamTrans *self)
{
    return self->height;
}
void
camtrans_get_distortion_center (const CamTrans *self, double *x, double *y)
{
    SphericalDistortion *dist_data = (SphericalDistortion*)self->dist_data;
    *x = dist_data->cx;
    *y = dist_data->cy;
}

int
camtrans_undistort_pixel (const CamTrans *self, const double x, const double y,
                          double *ox, double *oy)
{
    return self->undist_func (x, y, self->dist_data, ox, oy);
}

int
camtrans_distort_pixel (const CamTrans *self, const double x, const double y,
                        double *ox, double *oy)
{
    return self->dist_func (x, y, self->dist_data, ox, oy);
}

void
camtrans_unproject_undistorted(const CamTrans *self, double im_x, double im_y, 
                           double ray[3])
{
    double pixel_v[3] = {im_x, im_y, 1.};
    bot_matrix_vector_multiply_3x3_3d (self->inv_matx, pixel_v, ray);
}

int
camtrans_project_undistorted (const CamTrans *self, const double p[3],
                  double im_xyz[3])
{
    double temp[3];
    bot_matrix_vector_multiply_3x3_3d (self->matx, p, temp);

    if (fabs (temp[2]) < CAMERA_EPSILON) return -1;

    double inv_z = 1/temp[2];
    im_xyz[0] = temp[0]*inv_z;
    im_xyz[1] = temp[1]*inv_z;
    im_xyz[2] = temp[2];

    return 0;
}

void
camtrans_unproject_distorted(const CamTrans *self, double im_x, double im_y, 
                           double ray[3])
{
    double dx, dy;
    camtrans_undistort_pixel(self, im_x, im_y, &dx, &dy);
    camtrans_unproject_undistorted(self, dx, dy, ray);
}

int
camtrans_project_distorted(const CamTrans *self, const double p[3],
        double im_xyz[3])
{
    double uxyz[3];
    if(0 != camtrans_project_undistorted(self, p, uxyz))
        return -1;
    if(0 == camtrans_distort_pixel(self, uxyz[0], uxyz[1], &im_xyz[0], &im_xyz[1])) {
        im_xyz[2] = uxyz[2];
        return 0;
    }
    return -1;
}

void
camtrans_scale_image (CamTrans *self,
                      const double scale_factor)
{
    // Image size
    self->width *= scale_factor;
    self->height *= scale_factor;

    // Pinhole parameters
    self->cx *= scale_factor;
    self->cy *= scale_factor;
    self->fx *= scale_factor;
    self->fy *= scale_factor;
    self->skew *= scale_factor;

    // Projection matrix
    int i;
    for (i = 0; i < 8; ++i)
      self->matx[i] *= scale_factor;

    // Inverse projection matrix
    double inv_scale_factor = 1/scale_factor;
    for (i = 0; i < 3; ++i) {
      self->inv_matx[3*i+0] *= inv_scale_factor;
      self->inv_matx[3*i+1] *= inv_scale_factor;
    }

    // Distortion data
    SphericalDistortion* dist_data = (SphericalDistortion*)self->dist_data;
    dist_data->w *= scale_factor;
    dist_data->h *= scale_factor;
    dist_data->inv_w *= inv_scale_factor;
}
