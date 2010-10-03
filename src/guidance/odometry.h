#ifndef _GUIDANCE_ODOMETRY_H__
#define _GUIDANCE_ODOMETRY_H__

#include <stdlib.h>
#include <stdio.h>

#include "matcher.h"
#include "classifier.h"
#include "flow.h"

#include <bot/bot_core.h>

void odometry_compute_from_flow_set (flow_t **flow, int nflow, double *rot_speed, double *trans_speed, double *relative_z);
void odometry_classify_motion_from_flow (GQueue *motions, flow_t **flow, int num);

void odometry_compute (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2, double pix_to_rad, double scene_depth,
        double *rot_speed, double *trans_speed, double *relative_z);
void odometry_from_pose (botlcm_pose_t *p1, botlcm_pose_t *p2,
        double *rot_speed, double *trans_speed, double *relative_z);

#endif

