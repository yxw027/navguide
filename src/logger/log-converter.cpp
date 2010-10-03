/* 
 * This module converts images from the old format to the new (libbot) image format.
 * It listens to images on channels OLDIMAGE.*, converts them and re-publish them on
 * CAM_THUMB.*
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
#include <sys/statvfs.h>

#include <glib.h>

#include <lcm/lcm.h>

#include <common/glib_util.h>
#include <common/dbg.h>
#include <common/date.h>
#include <common/lcm_util.h>
#include <common/getopt.h>
#include <common/config_util.h>
#include <common/fileio.h>

#include <bot/bot_core.h>
#include <camunits/pixels.h>

struct state_t {
    GMainLoop *loop;
    lcm_t *lcm;
};

static state_t *g_self;

/* convert old image to new image format
 */
static void on_old_image (const lcm_recv_buf_t *rbuf, const char *channel, 
        const navlcm_image_old_t *msg, void *user_data )
{
    state_t *self = (state_t*)user_data;

    botlcm_image_t *img = (botlcm_image_t*)calloc(1, sizeof(botlcm_image_t));

    img->utime = msg->utime;
    img->width = msg->width;
    img->height = msg->height;
    img->row_stride = msg->stride;
    img->pixelformat = msg->pixelformat;
    img->size = msg->size;
    img->data = (unsigned char*)malloc(msg->size);
    memcpy (img->data, msg->image, msg->size);

    // convert color to grayscale
    if (msg->width * msg->height * 3 == msg->size) {
        unsigned char *data = (unsigned char*)calloc(msg->width * msg->height, 1);
        for (int i=0;i<msg->width*msg->height;i++) {
            double val = (1.0 * msg->image[3*i] + 1.0 * msg->image[3*i+1] + 1.0 * msg->image[3*i+2])/3;
            data[i] = MIN (255, MAX (0, math_round (val)));
        }
        free (img->data);
        img->data = data;
        img->size = img->width * img->height;
        img->pixelformat = CAM_PIXEL_FORMAT_GRAY;
        img->row_stride = img->width;
    }

    // metadata dictionary
    img->nmetadata = 0;
    img->metadata = NULL;

    int sensorid;
    if (sscanf (channel, "OLDIMAGE_%d", &sensorid)==1) {
        char newchannel[128];
        sprintf (newchannel, "CAM_THUMB_%d", sensorid);
        botlcm_image_t_publish (self->lcm, newchannel, img);
        printf ("%s --> %d x %d image to %s\n", channel, msg->width, msg->height, newchannel);
    
    } else {
        fprintf (stderr, "Unrecognized channel %s.\n", channel);
    }

    botlcm_image_t_destroy (img);
}

int main(int argc, char *argv[])
{
    dbg_init ();

    // set some defaults
    g_self = (state_t*)malloc(sizeof(state_t));
    state_t *self = g_self;

    self->lcm = lcm_create (NULL);
    if (!self->lcm) {
        fprintf (stderr, "Couldn't initialize LCM!");
        return -1;
    }

    getopt_t *gopt = getopt_create();

    getopt_add_bool   (gopt, 'h',   "help",    0,        "Show this help");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    navlcm_image_old_t_subscribe (self->lcm, "OLDIMAGE.*", on_old_image, self);

    self->loop = g_main_loop_new (NULL, FALSE);
    signal_pipe_glib_quit_on_kill (self->loop);
    glib_mainloop_attach_lcm (self->lcm);

    // main loop
    g_main_loop_run (self->loop);

    // cleanup
    glib_mainloop_detach_lcm (self->lcm);
}

