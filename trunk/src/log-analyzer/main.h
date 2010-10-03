#ifndef _CLASSIFIER_MAIN_H__
#define _CLASSIFIER_MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>
#include <math.h>
#include <poll.h>
#include <fcntl.h>

#include <glib.h>
#include <glib-object.h>

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm.h>
#include <bot/bot_core.h>

#include <glib.h>
#include <glib-object.h>

#include <bot/bot_core.h>

/* From common */
#include <common/getopt.h>
#include <common/dbg.h>
#include <common/codes.h>
#include <common/lcm_util.h>
#include <common/config_util.h>

/* from guidance */
#include <guidance/dijkstra.h>

struct state_t {

    lcm_t *lcm;
    GMainLoop *loop;
    GQueue *feature_list;   // local copy of features
    GQueue **camimg_queue;  // local copy of images

    config_t *config;

    GQueue *upward_image_queue;
    int64_t utime;

    gboolean debug;
    gboolean verbose;

    BotConf *conf;
    navlcm_class_param_t *class_state;

    int64_t mission_start_utime;
   
    int mission_start_node_id;
    int mission_target_node_id;
    int user_lost_count;
    int unclear_guidance_count;
    int total_user_lost_count;
    int total_unclear_guidance_count;
    int64_t last_user_lost_utime;
    int64_t last_unclear_guidance_utime;
    int n_missions;
    int n_missions_passed;
    int total_excursion_time;

    double total_speed_ratio;

    int64_t end_of_log_utime;

    dijk_graph_t *dg; 
};

state_t *g_self;

#endif
