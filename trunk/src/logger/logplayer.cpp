/* 
 * This module allows to replay a log file. The LCM library provides a logplayer,
 * but we had to write our own in order to add more controls. This code is highly
 * similar to the original LCM logplayer.
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>

#include <glib.h>
#include <glib-object.h>

#include <lcm/lcm.h>

/* from common */
#include <common/glib_util.h>
#include <common/dbg.h>
#include <common/getopt.h>
#include <common/codes.h>
#include <common/lcm_util.h>
#include <common/fileio.h>

/* from lcmtypes */
#include <lcmtypes/navlcm_logplayer_param_t.h>
#include <lcmtypes/navlcm_logplayer_state_t.h>

#include "logread.h"

struct logplayer_t
{
    GMainLoop *loop;
    lcm_logread_t *logread;
    lcm_t * lcm_out;
    lcm_t * lcm;

    GThread *replay_thread;
    //GMutex *mutex;
    //GCond *cond;
   
    char *file;
    int pause;
    int playing;
    int interrupt;
    float speed;
    int verbose;

    int forward_request;
    int rewind_request;
    int64_t event_time;
    int delta_time;
    int repeat;
    int exit;
};

static logplayer_t *g_logplayer;

int start_replay (logplayer_t *lp);
int stop_replay (logplayer_t *lp);
int pause_resume (logplayer_t *lp);
 
void
handler (const lcm_recv_buf_t *rbuf, const char *channel, void *u)
{
    logplayer_t * l = (logplayer_t*)u;

    if (l->verbose)
        dbg (DBG_INFO, "%.3f Channel %-20s size %d", rbuf->recv_utime / 1000000.0,
                channel, rbuf->data_size);

    lcm_publish (l->lcm_out, channel, rbuf->data, rbuf->data_size);
}

void 
forward (logplayer_t *lp)
{
    if (!lp->playing)
        return;

    lp->forward_request = 1;
}

void
rewind (logplayer_t *lp)
{
    if (!lp->playing)
        return;

    lp->rewind_request = 1;
}

static void
on_logplayer_param (const lcm_recv_buf_t *buf, const char *channel, const navlcm_logplayer_param_t *msg, 
                    void *user)
{
    dbg (DBG_INFO, "[logplayer] received logplayer param. code = %d", msg->code);

    switch (msg->code) {
        
    case LOGPLAYER_START_REPLAY:
        g_logplayer->file = strdup (msg->filename);
        g_logplayer->speed = msg->speed;
        dbg (DBG_INFO, "starting replay on %s, speed = %.3f", msg->filename, msg->speed);
        start_replay (g_logplayer);
        break;
    case LOGPLAYER_STOP_REPLAY:
        dbg (DBG_INFO, "[logplayer] stopping replay.");
        stop_replay (g_logplayer);
        break;
    case LOGPLAYER_PAUSE_RESUME:
        pause_resume (g_logplayer);
        break;
    default:
        dbg (DBG_ERROR, "[logplayer] unknown param code.");
        break;
    case LOGPLAYER_FORWARD:
        g_logplayer->delta_time = atoi (msg->filename);
        forward (g_logplayer);
        break;
    case LOGPLAYER_REWIND:
        g_logplayer->delta_time = atoi (msg->filename);
        rewind (g_logplayer);
        break;
        
    }
}

gpointer *replay_thread_cb (gpointer data)
{
    logplayer_t *lp = (logplayer_t*)data;

    int count = 0;
    
    while (!lp->exit && (lp->repeat == 0 || count < lp->repeat)) {

        // lcm_out
        const char *provider = "NULL";//"udpm://?transmit_only=true";
        
        lp->lcm_out = lcm_create (NULL); // used to be provider
        if (!lp->lcm_out) {
            fprintf (stderr, "Error: Failed to create %s\n", provider);
            return NULL;
        }
        
        // check that filename exists
        if (!lp->file) {
            dbg (DBG_ERROR, "[logplayer] no filename provided.");
            return NULL;//continue;
        }
        
        dbg (DBG_INFO, "[logplayer] received replay signal. file: %s", lp->file);
        
        
        // check that filename is valid
        if (!file_exists (lp->file)) {
            dbg (DBG_ERROR, "[logplayer] failed to replay %s."
                 " File does not exist.", lp->file);
            return NULL;
        }
        
        // create logread
        if (!lp->logread) {
            char url_in[strlen(lp->file) + 64];
            sprintf (url_in, "file://%s?speed=%f", lp->file, lp->speed);
            //lp->lcm_in = lcm_create (url_in);
            lp->logread = logread_create (lp->lcm_out, url_in);
            //        lcm_subscribe (lp->logread->lcm, ".*", handler, lp);
            dbg (DBG_INFO, "[logplayer] %s", url_in);
        }
        
        // check that logread exists
        if (!lp->logread) {
            dbg (DBG_ERROR, "[logplayer] logread not initialized.");
            return NULL;//continue;
        }

        // resume logread
        //if (lp->pause) 
        //    logread_resume (lp->logread, lp->event_time);
        
        // start replay
        lp->interrupt = 0;
        lp->pause = 0;
        lp->playing = 1;
        
        while (!lp->interrupt) {
            if (lp->forward_request) {
                logread_seek_to_relative_time (lp->logread, lp->delta_time);
                lp->forward_request = 0;
            }
            
            if (lp->rewind_request) {
                logread_seek_to_relative_time (lp->logread, -lp->delta_time);
                lp->rewind_request = 0;
            }
            
            if (lp->pause) {
                usleep (1000000);
                logread_offset_next_clock_time (lp->logread, 1000000);
            } else {
                if (logread_handle (lp->logread)<0) {
                    dbg (DBG_INFO, "[logplayer] EOF");
                    lp->interrupt = 1;
                }        
                lp->event_time = logread_event_time (lp->logread);
            }
        }

        lp->playing = 0;
    
        if (lp->file) {
            free (lp->file);
            lp->file = NULL;
        }

        // destroy lcm_in
        logread_destroy (lp->logread);
        lp->logread = NULL;

        // emit end-of-log signal
        uint8_t dummy = 1;
        lcm_publish (g_logplayer->lcm, "LOGGER_END_OF_LOG", 
                &dummy, sizeof(uint8_t));

        count++;
    }

    return NULL;
}

int start_replay (logplayer_t *lp)
{
    if (!lp->playing) {

        // stop grabber
        // it's up to the user to resume grabbing
        //
        publish_grabber_param (lp->lcm, GRABBER_STOP, 0, 0, 0);

        // start replay thread
        g_logplayer->replay_thread = g_thread_create
            ((GThreadFunc)replay_thread_cb, g_logplayer, TRUE, NULL);

    } else {

        dbg (DBG_ERROR, "[logplayer] error: can't start replay. "
                "already playing...");
    }

    return 0;
}

int stop_replay (logplayer_t *lp)
{
    if (lp->playing) {
        lp->pause = 0;
        lp->interrupt = 1;
    } else {
        lp->pause = 0;
        lp->interrupt = 0;
        if (lp->logread) {
            logread_destroy (lp->logread);
            lp->logread = NULL;
        }
    }


    return 0;
}

int pause_resume (logplayer_t *lp)
{
    lp->pause = !lp->pause;

    return 0;
}

gboolean publish_logplayer_state (gpointer data)
{
    navlcm_logplayer_state_t state;
    state.filename = g_logplayer->file ? strdup (g_logplayer->file) : strdup ("");
    navlcm_logplayer_state_t_publish (g_logplayer->lcm, "LOGPLAYER_STATE", &state);
    free (state.filename);

    return TRUE;
}

    int
main(int argc, char ** argv)
{

    dbg_init ();

    // init thread
    if (!g_thread_supported ()) {
        g_thread_init (NULL);
        dbg (DBG_INFO, "[logplayer] g_thread supported");
    } else {
        dbg (DBG_ERROR,"[logplayer] g_thread NOT supported");
        exit (1);
    }

    // set some defaults
    g_logplayer = (logplayer_t*)malloc(sizeof(logplayer_t));
    g_logplayer->playing = 0;

    // command line arguments
    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,        "Be verbose");
    getopt_add_string (gopt, 'f', "logfile", "", "Log file name");
    getopt_add_string (gopt, 's', "speed", "1.0", "Replay speed");
    getopt_add_int    (gopt, 'r', "repeat", "1",  "Repeat some number of times. 0 for infinity.");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    g_logplayer->verbose = getopt_get_bool (gopt, "verbose");

    const char *file = getopt_get_string (gopt, "logfile");
    g_logplayer->file = NULL;
    g_logplayer->speed = (float)atof(getopt_get_string (gopt, "speed"));
    g_logplayer->forward_request = 0;
    g_logplayer->rewind_request = 0;
    g_logplayer->delta_time = 0;
    g_logplayer->repeat = getopt_get_int (gopt, "repeat");
    g_logplayer->exit = 0;

    // lcm
    g_logplayer->logread = NULL;

    g_logplayer->lcm = lcm_create (NULL);
    glib_mainloop_attach_lcm (g_logplayer->lcm);

    // subscribe to LOGPLAYER_PARAM
    navlcm_logplayer_param_t_subscribe (g_logplayer->lcm, "LOGPLAYER_PARAM",
            on_logplayer_param, g_logplayer);

    g_timeout_add (5000, &publish_logplayer_state, NULL);

    // replay a log file
    if (strlen (getopt_get_string (gopt, "logfile")) > 2) {
        g_logplayer->file = strdup (file);
        start_replay (g_logplayer);
    } 

    g_logplayer->loop = g_main_loop_new (NULL, FALSE);
    signal_pipe_glib_quit_on_kill (g_logplayer->loop);

    // run main loop
    g_main_loop_run (g_logplayer->loop);

    dbg (DBG_INFO, "quitting main loop.");

    // unlock replay thread
    stop_replay (g_logplayer);

    g_logplayer->exit = 1;

    // wait for thread to finish
    g_thread_join (g_logplayer->replay_thread);

    return 0;
}


