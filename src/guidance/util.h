#ifndef _CLASS_UTIL_H__
#define _CLASS_UTIL_H__

#include <stdio.h>
#include <stdlib.h>

/* from LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_dictionary_t.h>

#include <common/stringutil.h>

#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippcc.h"
#include "ippj.h"

#include <camunits/cam.h>

/* from here */
#include "gates.h"
#include <jpegcodec/pngload.h>

typedef struct {
    int64_t utime;
    int nodeid;
} ground_truth_t;

void class_test_ui (lcm_t *lcm);
void class_publish_roadmap (lcm_t *lcm, GQueue *gates);
void class_run_matching_analysis (GQueue **images, int nsensors, GQueue *features, int64_t utime1, int64_t utime2, lcm_t *lcm);
void class_publish_map_list (lcm_t *lcm);
void class_stitch_image_to_file (GQueue **img, int nimg, const char *filename);
void save_to_image (GQueue *img, navlcm_feature_list_t *features,  const char *filename);
void read_mission_file (GQueue *mission, const char *filename);
void util_compute_statistics (GQueue *poses);
GQueue *util_read_ground_truth (const char *filename);
GQueue *util_read_poses (const char *filename);
int util_fix_pose (botlcm_pose_t **pose, GQueue *poses);
void util_publish_image (lcm_t *lcm, botlcm_image_t *img, const char *channel, int width, int height);
void util_publish_map_list (lcm_t *lcm, const char *prefix, const char *suffix, const char *channel) ;

#endif

