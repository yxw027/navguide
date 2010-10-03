/*
 * This module synchronizes features observed on multiple cameras. 
 * It listens to FEATURE messages and generates FEATURE_SET messages.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <poll.h>
#include <fcntl.h>

#include <glib.h>
#include <glib-object.h>

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_feature_list_t.h>

/* From common */
#include <common/getopt.h>
#include <common/dbg.h>
#include <common/glib_util.h>
#include <common/date.h>
#include <common/codes.h>
#include <common/timestamp.h>
#include <common/lcm_util.h>
#include <common/config_util.h>

struct state_t {

    GMainLoop *loop;
    gboolean verbose;
    lcm_t *lcm;

    navlcm_feature_list_t **set;
    char *mark;

    config_t *config;
    GTimer *timer;
};

state_t *g_self;

// save features to local memory
//
static void
on_feature_list_event (const lcm_recv_buf_t *buf, const char *channel, 
                           const navlcm_feature_list_t *msg, 
                           void *user)
{
    state_t *self = (state_t*)user;

    int sensor_id = msg->sensorid;

    //dbg (DBG_INFO, "[collector] received features for sensor %d", sensor_id);

    if (self->config->nsensors <= sensor_id)
        return;

    // announce feature list received
    navlcm_generic_cmd_t cmd;
    cmd.code = sensor_id;
    cmd.text = (char*)"";
    navlcm_generic_cmd_t_publish (self->lcm, "FEATURES_ANNOUNCE", &cmd);
    dbg (DBG_INFO, "received feature set for sensor %d", sensor_id);

    // copy to local memory
    if (self->set[sensor_id])
        navlcm_feature_list_t_destroy (self->set[sensor_id]);

    self->set[sensor_id] = navlcm_feature_list_t_copy (msg);
    
    // increment mark
    self->mark[sensor_id] = 1;

    // if a whole set has been received, publish
    int whole_set = 1;
    for (int i=0;i<self->config->nsensors;i++) {
        if (self->mark[i] == 0) {
            whole_set = 0;
            break;
        }
    }

    if (whole_set) {

        //publish feature set

        navlcm_feature_list_t *f = navlcm_feature_list_t_copy (self->set[0]);

        for (int i=1;i<self->config->nsensors;i++) {
            f = navlcm_feature_list_t_append (f, self->set[i]);
        }

        navlcm_feature_list_t_publish ( self->lcm, "FEATURE_SET", f);

        // reset markers
        for (int i=0;i<self->config->nsensors;i++)
            self->mark[i] = 0;

        // free memory
        navlcm_feature_list_t_destroy (f);

        // compute apparent frame rate
	    double secs = g_timer_elapsed (self->timer, NULL);
        g_timer_start (self->timer);
        
        dbg (DBG_INFO, "[collector] publishing feature set for "
	    "timestamp %ld (frame rate: %.2f Hz)", msg->utime, 1.0 / secs);
    }
}


// the main method
//
int main(int argc, char *argv[])
{
    g_type_init ();

    dbg_init ();

    // initialize application state and zero out memory
    g_self = (state_t*) calloc( 1, sizeof(state_t) );

    // run the main loop
    g_self->loop = g_main_loop_new(NULL, FALSE);

    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,     "Be verbose");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }
    
    g_self->verbose = getopt_get_bool(gopt, "verbose");

    // read config file
    if (!(g_self->config = read_config_file ())) {
        //dbg (DBG_ERROR, "[collector] failed to read config file.");
        return 0;
    }

    g_self->timer = g_timer_new();

    g_self->set = (navlcm_feature_list_t **)
                   malloc(g_self->config->nsensors*sizeof(navlcm_feature_list_t*));
    g_self->mark = (char*)malloc(g_self->config->nsensors*sizeof(char));
    
    for (int i=0;i<g_self->config->nsensors;i++) {
        g_self->set[i] = NULL;
        g_self->mark[i] = 0;
    }
    
    g_self->lcm = lcm_create(NULL);
    if (!g_self->lcm)
        return 1;

    //dbg (DBG_INFO, "[collector] init for %d sensors...", g_self->config->nsensors);

    // subscribe to FEATURES messages
    navlcm_feature_list_t_subscribe (g_self->lcm, "FEATURES", 
                                      on_feature_list_event, g_self);

    // attach the kill signal
    signal_pipe_glib_quit_on_kill (g_self->loop);

    // attach lcm to main loop
    glib_mainloop_attach_lcm (g_self->lcm);

    // run the main loop
    g_main_loop_run (g_self->loop);

    // cleanup
    g_main_loop_unref (g_self->loop);

    return 0;
}
