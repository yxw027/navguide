#ifndef FEATURES_UTIL_H__
#define FEATURES_UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

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

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_feature_list_t.h>
#include <lcmtypes/navlcm_features_param_t.h>
#include <bot/bot_core.h>

/* From camunits */
#include <camunits/cam.h>
#include <dc1394/dc1394.h>


/* From common */
#include <common/getopt.h>
#include <common/dbg.h>
#include <common/timestamp.h>
#include <common/glib_util.h>
#include <common/lcm_util.h>
#include <common/codes.h>
#include <common/config_util.h>

/* From libsift */
#include <libsift/sift.h>

/* from libfast */
#include <libfast/fast.h>

/* From libsift2 */
#include <libsift2/sift.hpp>

/* from libsurf */
#include <libsurf/surflib.h>

/* From libmser */
#include <libmser/mser.h>

/* from image */
#include <image/image.h>

/* From GLIB */
#include <glib.h>
#include <glib-object.h>

/* From jpeg */
#include <jpegcodec/jpegload.h>
#include <jpegcodec/ppmload.h>

navlcm_feature_list_t * features_driver (botlcm_image_t *img, int sensorid, navlcm_features_param_t *param, gboolean save_to_file);
navlcm_feature_list_t *features_driver (botlcm_image_t **img, int nimg, navlcm_features_param_t *param);

#endif

