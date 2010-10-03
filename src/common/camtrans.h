#ifndef __CAMTRANS_H__
#define __CAMTRANS_H__

#include <common/geometry.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Camera representation
//

typedef struct _CamTrans CamTrans;

CamTrans* camtrans_new_spherical (const char *name,
                   double width, double height,
                   double fx, double fy, double cx, double cy, double skew,
                   const double distortion_cx, const double distortion_cy,
                   double distortion_param);

/**
 * camtrans_destroy:
 *
 * Destructor. Releases resources allocated by camtrans_new_spherical().
 */
void camtrans_destroy (CamTrans *t);

double camtrans_get_focal_length_x (const CamTrans *t);
double camtrans_get_focal_length_y (const CamTrans *t);
double camtrans_get_image_width (const CamTrans *t);
double camtrans_get_image_height (const CamTrans *t);
double camtrans_get_principal_x (const CamTrans *t);
double camtrans_get_principal_y (const CamTrans *t);
double camtrans_get_width (const CamTrans *t);
double camtrans_get_height (const CamTrans *t);
void camtrans_get_distortion_center (const CamTrans *t, double *x, double *y);
const char *camtrans_get_name(const CamTrans *t);

/**
 * camtrans_undistort_pixel_exact:
 *
 * Produce rectified pixel from distorted pixel. Returns 0 upon success.
 */
int camtrans_undistort_pixel (const CamTrans *t, const double x, const double y,
                              double *ox, double *oy);

/**
 * camtrans_distort_pixel:
 *
 * Produce distorted pixel from rectified pixel. Returns 0 upon success.
 */
int camtrans_distort_pixel (const CamTrans *t, const double x, const double y,
                            double *ox, double *oy);

/**
 * camtrans_unproject_undistorted:
 *
 * 
 */
void camtrans_unproject_undistorted(const CamTrans *t, double im_x, 
        double im_y, double ray[3]);

/** camtrans_project_undistorted:
 *
 * Projects the 3D point @p in the camera coordinate frame to the image plane.
 * Computes undistorted pixel coordinates.  Computes depth of projected point.
 *
 * @t CamTrans object to project and undistort with.
 * @p Point to project to camera.
 * @im_xyz (x,y) coordinate on image plane (z=1) and z = depth.
 *
 * Returns 0 upon success. Error results (-1 return value) if the
 * point @p lies on the camera's principal plane.
 */
int camtrans_project_undistorted(const CamTrans *t, const double p[3], 
        double im_xyz[3]);

// convenience function.  
void camtrans_unproject_distorted(const CamTrans *t, double im_x, 
        double im_y, double ray[3]);

// convenience function.  
int camtrans_project_distorted(const CamTrans *self, const double p[3],
        double im_xyz[3]);

void camtrans_scale_image (CamTrans *t, const double scale_factor);

#ifdef __cplusplus
}
#endif

#endif
