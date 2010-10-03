#ifndef _CLASS_ROTATION_H__
#define _CLASS_ROTATION_H__

#include <stdio.h>
#include <stdlib.h>

/* From IPP */
#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippcc.h"
#include "ippj.h"

/* From OpenCV */
#include <opencv/cv.h>
#include <opencv/highgui.h>

// from lcmtypes
#include <lcmtypes/navlcm_feature_list_t.h>

/* from tracker */
#include "matcher.h"

/* from common */
#include <common/mathutil.h>
#include <common/lcm_util.h>
#include <common/geometry.h>

/* From libsift */
#include <libsift/sift.h>

/* from libjpeg */
#include <jpegcodec/ppmload.h>

int rot_estimate_angle_from_images (botlcm_image_t *img1, botlcm_image_t *img2, double *angle_rad, double *stdev);
int rot_estimate_angle_from_images (const char *f1, const char *f2);

#endif

