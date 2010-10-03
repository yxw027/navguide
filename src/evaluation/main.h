#ifndef _MAIN_H__
#define _MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

/* from here */
#include "events.h"
#include "draw.h"

/* From common */
#include <common/getopt.h>
#include <common/dbg.h>
#include <common/texture.h>
#include <gtk_util/gtk_util.h>
#include <gtk_util/gtkgldrawingarea.h>
#include <common/glib_util.h>
#include <common/gltool.h>
#include <common/getopt.h>
#include <common/fileio.h>

/* From the GL library */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

/* from LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_features_param_t.h>
#include <lcmtypes/navlcm_feature_match_set_t.h>
#include <bot/bot_core.h>

#ifndef GLADE_TARGET_PATH
#error "GLADE_TARGET_PATH must be defined!!!"
#endif
#define GLADE_FILE GLADE_TARGET_PATH "/evaluation.glade"

#define NSENSORS 4
static const char *CHANNEL_NAMES[NSENSORS] = { "CAM_THUMB_RL", "CAM_THUMB_FL", "CAM_THUMB_FR", "CAM_THUMB_RR"};

#define SIFT 0
#define SURF 1

#define SIFT_NSCALES 3
#define SIFT_SIGMA  1.6
#define SIFT_PEAKTHRESH 0.06
#define SIFT_DOUBLEIMSIZE 0

#define SURF_OCTAVES    3
#define SURF_INTERVALS  4
#define SURF_INIT_SAMPLE    2
#define SURF_THRESH     0.0012

#define FAST_THRESH 45

#define MATCH_THRESH 0.6

struct state_t {
    GladeXML *gladexml;
    GtkuGLDrawingArea *gl_area_1;
    lcm_t *lcm;
    botlcm_image_t *img[NSENSORS];

    botlcm_image_t *frame1[NSENSORS];
    botlcm_image_t *frame2[NSENSORS];
    navlcm_features_param_t *features_param;
    navlcm_feature_list_t *features1;
    navlcm_feature_list_t *features2;
    navlcm_feature_list_t *features1c;
    navlcm_feature_list_t *features2c;
    navlcm_feature_match_set_t *matches; // auto matches
    navlcm_feature_match_set_t *gmatches; // ground-truth matches
    navlcm_feature_match_set_t *mismatches;

    navlcm_feature_match_t *highlight_m;
    navlcm_feature_t *highlight_f;
    navlcm_feature_t *match_f1;
    navlcm_feature_t *match_f2;
    navlcm_feature_match_t *select_m;

    botlcm_pose_t *pose;
    botlcm_pose_t *pose1;
    botlcm_pose_t *pose2;

    double mouse_x, mouse_y;

    double user_scale_1;
    double user_trans_1[2];

    GQueue *history;
    gboolean feature_finder_pressed;
};

static state_t *g_self;
void set_background_color (state_t *self);;
void set_param_defaults (state_t *self, int type);
void clear_matches (state_t *self);
void clear_features (state_t *self, gboolean frame1, gboolean frame2);
void clear_data (state_t *self, gboolean frame1, gboolean frame2);
void read_data_from_file (state_t *self, const char *filename);
void save_data_to_file (state_t *self, const char *filename);
void duplicate_features (state_t *self);
void select_keypoint (state_t *self);
gboolean delete_keypoint (state_t *self);
gboolean delete_match (state_t *self);
void add_match (state_t *self);
void add_match (state_t *self, navlcm_feature_match_t *match);
void replace_match (state_t *self, navlcm_feature_match_t *match);
void select_match (state_t *self);
void remove_match (state_t *self, navlcm_feature_t *f);
void util_compute_mismatches (state_t *self);
void history_pop (state_t *self);
void history_push (state_t *self, int code, gpointer data);
void util_on_key_press (state_t *self, int keyval);
void util_on_key_release (state_t *self, int keyval);
void util_on_button_press (state_t *self, int button);
void util_span_matching_threshold (state_t *self);
navlcm_feature_match_set_t *util_subtract_sets (navlcm_feature_match_set_t *set1, navlcm_feature_match_set_t *set2);
gboolean match_equal (navlcm_feature_match_t *m1, navlcm_feature_match_t *m2);

double compute_recall (navlcm_feature_match_set_t *set1, navlcm_feature_match_set_t *set2);
double compute_precision (navlcm_feature_match_set_t *set1, navlcm_feature_match_set_t *set2);

#endif

