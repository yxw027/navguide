/*
 * This module computes various types of image features (sift, fast, mser, etc.) and relies
 * on the corresponding libraries (libsift2, libfast, libmser, etc.)
 *
 */
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
#include <lcmtypes/camlcm_image_t.h>

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
#include <jpegcodec/pngload.h>

#define WWW_HOME "/home/agile/www/"

struct state_t {

    lcm_t *lcm;
    GMainLoop *loop;

    config_t *config;
    char *channel_name;
    GHashTable *hash_utime;

    camlcm_image_t *img;
};

state_t *g_self;

    static void
on_camlcm_image_event (const lcm_recv_buf_t *buf, const char *channel, 
        const camlcm_image_t *msg,
        void *user)
{
    state_t *self = g_self;

    if (self->img) camlcm_image_t_destroy (self->img);
    self->img = camlcm_image_t_copy (msg);

    dbg (DBG_INFO, "received image on channel %s...", channel);

    // fetch last utime for this channel
    // and return if it is not old enough
    gpointer data = g_hash_table_lookup (self->hash_utime, channel);
    if (data) {
        int64_t *cp = (int64_t*)data;
        if (msg->utime - *cp < 2000000)
            return;
    }

    char filename[512];
    sprintf (filename, "%s/img/%s.png", WWW_HOME, channel);

    dbg (DBG_INFO, "saving to %s.", filename);

    write_png (msg->data, msg->width, msg->height, msg->row_stride / msg->width, filename);

    // save utime to hash table
    int64_t *cp = (int64_t*)malloc(sizeof(int64_t));
    *cp = msg->utime;

    g_hash_table_replace (self->hash_utime, strdup (channel), cp);
}

void g_int64_free (gpointer data)
{
    free (data);
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
    self->img = NULL;
    self->hash_utime = g_hash_table_new_full (g_str_hash , g_str_equal , (GDestroyNotify)g_str_free, (GDestroyNotify)g_int64_free);

    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,     "Be verbose");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        dbg (DBG_FEATURES,"Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    if (!(self->config = read_config_file ())) {
        dbg (DBG_ERROR, "[features] failed to read config file.");
        return 0;
    }

    self->lcm = lcm_create(NULL);
    if (!self->lcm)
        return 1;

    // subscribe to the image channel
    for (int i=0;i<self->config->nsensors;i++) {
        camlcm_image_t_subscribe (self->lcm, self->config->channel_names[i], 
                on_camlcm_image_event, self);
    }

    // attach LCM to the main loop
    glib_mainloop_attach_lcm (self->lcm);
    signal_pipe_glib_quit_on_kill (self->loop);

    g_main_loop_run (self->loop);

    // cleanup
    g_main_loop_unref (self->loop);

    return 0;
}
