#ifndef _LCM_SIFT_H__
#define _LCM_SIFT_H__

#include <stdio.h>
#include <stdlib.h>

/* From IPP */
#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippcc.h"
#include "ippj.h"

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_sift_t.h>

//#include <common/getopt.h>
#include <common/dbg.h>

/* From libsift */
#include "sift.h"

int
lcm_compute_sift ( unsigned char *data, int width, int height,
                   int nchannels, int sid, int64_t timestamp,
                   double resize, double crop, int doubleimsize,
                   int nscales, double peakthresh, double sigma,
                   SiftImage sift_img, lcm_t *lcm);

#endif
