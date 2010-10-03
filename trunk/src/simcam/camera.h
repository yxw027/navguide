#ifndef _CAMERA_H__
#define _CAMERA_H__

/* From the standard C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <math.h>

#include "math/quat.h"

typedef struct CamExtr { Quaternion q; Vec3f t; };

Mat3f HomographyFromPlane( const CamExtr c1, const CamExtr c2, const Vec3f p, const Vec3f n);

#endif
