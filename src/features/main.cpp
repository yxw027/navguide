/*
 * This module computes various types of image features (sift, fast, mser, etc.) and relies
 * on the corresponding libraries (libsift2, libfast, libmser, etc.)
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "util.h"

struct state_t {
    SiftImage sift_img;
    Ipp8u *sift_ipp_roi_img;

    navlcm_features_param_t *param;

    lcm_t *lcm;
    GMainLoop *loop;
    int sensorid;
    char stop;
    gboolean replay;
    int utime_offset;

    config_t *config;
    char *channel_name;

    botlcm_image_t *img;
    gboolean computing;
    gboolean save_image_to_pgm; // set to true to save the image to pgm
};

state_t *g_self;

gboolean publish_features_param (gpointer data)
{
    state_t *self = (state_t*)data;

    navlcm_features_param_t_publish (self->lcm, "FEATURES_PARAM", self->param);

    return TRUE;
}

    static void
on_features_param_event (const lcm_recv_buf_t *buf, const char *channel, 
        const navlcm_features_param_t *msg,
        void *user)
{
    state_t *self = (state_t*)user;

    if (msg->code == FEATURES_PARAM_CHANGED) {
        if (self->param)
            navlcm_features_param_t_destroy (self->param);
        self->param = navlcm_features_param_t_copy (msg);
    } else if (msg->code == FEATURES_STOP) {
        self->param->enabled = FALSE;
    } else if (msg->code == FEATURES_RESUME) {
        self->param->enabled = TRUE;
    } else if (msg->code == FEATURES_SAVE_TO_PGM) {
        self->save_image_to_pgm = TRUE;
    }

    return;
}

void* compute_thread_cb (void* data);

    static void
on_botlcm_image_event (const lcm_recv_buf_t *buf, const char *channel, 
        const botlcm_image_t *msg,
        void *user)
{
    state_t *self = g_self;

    // announce image received
    navlcm_generic_cmd_t cmd;
    cmd.code = self->sensorid;
    cmd.text = (char*)"";
    navlcm_generic_cmd_t_publish (self->lcm, "IMAGE_ANNOUNCE", &cmd);

    if (!self->param->enabled)
        return;

    if (self->computing) { dbg (DBG_FEATURES, "skipping  frame..."); return;}

    if (self->img) botlcm_image_t_destroy (self->img);
    self->img = botlcm_image_t_copy (msg);

    g_thread_create (compute_thread_cb, NULL, FALSE, NULL);

}

void* compute_thread_cb (void* data)
{
    state_t *self = g_self;

    self->computing = TRUE;

    // compute features
    navlcm_feature_list_t *features = features_driver (self->img, self->sensorid, self->param, self->save_image_to_pgm);

    // publish features
    if (features) {
        dbg (DBG_FEATURES, "[features] publishing [%d] features [type %d] for sensor %d", features->num, self->param->feature_type, self->sensorid);

        dbg (DBG_FEATURES, "size: %d", features->num);
        if (features->num > 0) {
            double max=.0;
            for (int i=0;i<features->el[0].size;i++)
                max  = fmax (max, features->el[0].data[i]);
            dbg (DBG_FEATURES, "max: %.6f", max);
        }

        navlcm_feature_list_t_publish (self->lcm, "FEATURES", features);
    }

    if (features)
        navlcm_feature_list_t_destroy (features);

    self->computing = FALSE;

    return NULL;
}

int main(int argc, char *argv[])
{
    dbg_init ();

    g_thread_init (NULL);

    // init random machine
    srand (time (NULL));

    // initialize application state and zero out memory
    g_self = (state_t*) calloc( 1, sizeof(state_t) );

    state_t *self = g_self;

    // run the main loop
    self->loop = g_main_loop_new(NULL, FALSE);

    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,     "Be verbose");
    getopt_add_int (gopt,   'o', "sensor-id", "-1", "Sensor ID - ");
    getopt_add_bool (gopt, 'g', "save-image", 0, "Save input images to PGM");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        dbg (DBG_FEATURES,"Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    if (!(self->config = read_config_file ())) {
        dbg (DBG_ERROR, "[features] failed to read config file.");
        return 0;
    }

    self->param = (navlcm_features_param_t*)malloc(sizeof(navlcm_features_param_t));

    // sift parameters
    self->param->sift_doubleimsize = self->config->sift_doubleimsize;
    self->param->sift_nscales = self->config->sift_nscales;
    self->param->sift_peakthresh = self->config->sift_peakthresh;
    self->param->sift_sigma = self->config->sift_sigma;

    // gftt parameters
    self->param->gftt_quality =      self->config->gftt_quality;
    self->param->gftt_min_dist =     self->config->gftt_min_dist;
    self->param->gftt_block_size =   self->config->gftt_block_size;
    self->param->gftt_use_harris =   self->config->gftt_use_harris;
    self->param->gftt_harris_param = self->config->gftt_harris_param;

    // surf parameters
    self->param->surf_octaves = self->config->surf_octaves;
    self->param->surf_intervals = self->config->surf_intervals;
    self->param->surf_init_sample = self->config->surf_init_sample;
    self->param->surf_thresh = self->config->surf_threshold;
   
    // fast parameters
    self->param->fast_thresh = self->config->fast_threshold;

    self->param->scale_factor = self->config->features_scale_factor;
    self->param->feature_type = self->config->features_feature_type;
    self->param->enabled = TRUE;

    self->sift_img = NULL;
    self->sift_ipp_roi_img = NULL;
    self->stop = 0;
    self->replay = FALSE;//getopt_get_bool (gopt, "replay");
    self->utime_offset = 0;
    self->channel_name = NULL;
    self->computing = FALSE;
    self->save_image_to_pgm = getopt_get_bool (gopt, "save-image");

    self->lcm = lcm_create(NULL);
    if (!self->lcm)
        return 1;

    self->sensorid = getopt_get_int (gopt, "sensor-id");

    assert (self->sensorid >= 0);

    assert (self->sensorid < self->config->nsensors);
    self->channel_name = self->config->channel_names[self->sensorid];
    assert (self->channel_name);

    // listen to sift param messages
    navlcm_features_param_t_subscribe (self->lcm, "FEATURES_PARAM_SET", &on_features_param_event, self);

    dbg (DBG_FEATURES, "listening to channel %s", self->channel_name);

    // listen to camlcm images
    botlcm_image_t_subscribe (self->lcm, self->channel_name, 
            on_botlcm_image_event, self);

    // publish features param regularly
    g_timeout_add_seconds (1, publish_features_param, self);

    // attach LCM to the main loop
    glib_mainloop_attach_lcm (self->lcm);
    signal_pipe_glib_quit_on_kill (self->loop);

    g_main_loop_run (self->loop);

    // cleanup
    g_main_loop_unref (self->loop);

    return 0;
}
