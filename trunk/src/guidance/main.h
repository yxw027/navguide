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

/* From common */
#include <common/getopt.h>
#include <common/dbg.h>
#include <common/glib_util.h>
#include <common/mathutil.h>
#include <common/mkl_math.h>
#include <common/codes.h>
#include <common/lcm_util.h>
#include <common/timestamp.h>
#include <common/config_util.h>
#include <common/applanix.h>
#include <common/globals.h>

/* From matcher */
#include "matcher.h"

/* From here */
#include "state.h"
#include "classifier.h"
#include "bags.h"
#include "dijkstra.h"
#include "loop.h"
#include "util.h"
#include "tracker2.h"
#include "flow.h"

/* from features */
#include <features/util.h>

#define BUFFSIZE 100
#define UI_VIDEO_MODE_LIVE_STREAM 0
#define UI_VIDEO_MODE_NAVIGATION 1
#define MAX_POSES 1000
#define CONFIDENCE_THRESH .4
#define ROTATION_DERIVATIVE_THRESH 1.0 // in radians
struct state_t {

    lcm_t *lcm;
    GMainLoop *loop;
    GQueue *feature_list;   // local copy of features
    GQueue **camimg_queue;  // local copy of images

    navlcm_class_param_t *param;

    GMutex *compute_mutex;
    GCond *compute_cond;
    GMutex *data_mutex;
    gboolean exit;

    gboolean ground_truth_enabled;

    int64_t gt_request_utime;
    int64_t utime1, utime2;
    int image_width, image_height;
    int features_width, features_height;

    config_t *config;
    gboolean computing;

    // graph representation of the nodes
    dijk_graph_t *d_graph;
    dijk_edge_t *current_edge;
    dijk_node_t *current_node;
    GQueue *path;

    char *seq_filename;
    gboolean save_images;


    int64_t last_audio_utime;

    GQueue *upward_image_queue;
    gboolean ground_thread_running;

    // ground truth position (DGC datasets)
    GQueue *pose_queue;
    navlcm_gps_to_local_t *gps_to_local;
    GQueue *applanix_data;
    GQueue *imu_queue;

    // reference point features (demo)
    navlcm_feature_list_t *ref_point_features;

    // directory where to save node images
    char *nodes_snap_dir;

    // user interface
    int64_t last_utterance_utime;
    int64_t utime;
    int64_t user_where_next_utime;

    // loop closure
    double *corrmat;
    int corrmat_size;
    GQueue *voctree; // a list of bag_t words

    int64_t last_node_estimate_utime;
    int64_t last_rotation_guidance_utime;
    navlcm_features_param_t *features_param;

    GQueue *mission;
    gboolean debug;
    gboolean verbose;

    BotConf *conf;
    int calibration_step;

    flow_field_set_t *flow_field_set;
    GQueue *flow_field_types;
    GQueue **flow_queue;
    char *flow_field_filename;
    double flow_scale;
    int flow_integration_time;

    motion_classifier_t *mc;

    GThread *compute_thread;

    bot_lcmgl_t *lcmgl;

    GQueue *fix_poses;

    GQueue *class_hist;

    int future_directions[3];

    int64_t end_of_log_utime;
};

state_t *g_self;

int utime_to_index (state_t *self, int64_t utime);
int64_t index_to_utime (state_t *self, int index);
double calibration_dead_reckoning (int delta, int nframes, 
                                                   double freq, int code);
void process_correlation_matrix (state_t *self, const char *filename, dijk_graph_t *dg);
void run_calibration (state_t *self, int code);
void populate_classifier_tables (state_t *self);
void run_imu_validation (state_t *self);
gboolean compute_dir_at_node (state_t *self, const char *filename);
void compute_foe (state_t *self);
gboolean publish_class_settings (gpointer data);
void reset_data (state_t *self);
void process_new_node (state_t *self, int64_t time1, int64_t time2);
void publish_ui_map (dijk_graph_t *dg, const char *channel, lcm_t *lcm);

void class_checkpoint_create (state_t *self, int64_t utime);
void class_checkpoint_revisit (state_t *self, int64_t utime);
void class_set_reference_point (state_t *self);
void save_class_state (state_t *self);
void utter_directions (state_t *self, double angle_rad);
gpointer demo_timeout_thread_func (gpointer data);
gpointer node_trigger_timeout_thread_func (gpointer data);
void on_class_ui_video_mode_changed (state_t *self, char *string);
void on_class_check_calibration (state_t *self);
void on_class_node_set_label (state_t *self, char *txt);
void on_class_add_node (state_t *self);
void on_class_start_exploration (state_t *self);
void on_class_end_exploration (state_t *self);
void on_class_homing (state_t *self);
void on_class_replay (state_t *self);
void on_blit_request (state_t *self);
void on_class_load_map (state_t *self, char *name);
void on_class_standby (state_t *self);
void on_class_dump_features (state_t *self);
void on_class_calibration_step (state_t *self, int step);
void on_class_user_where_next (state_t *self);

void class_goto_node (state_t *self, int id);

/* callback methods */
void motion_classifier_update_cb (state_t *self);
gpointer compute_thread_cb (gpointer data);
gboolean rotation_guidance_cb (gpointer data);
gboolean node_estimation_cb (gpointer data);
gpointer demo_cb (gpointer data);
gboolean publish_class_state_cb (gpointer data);
gpointer calibration_thread_cb (gpointer data);
gboolean calibration_check_cb (gpointer data);

#endif
