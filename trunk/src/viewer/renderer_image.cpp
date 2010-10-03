#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <lcm/lcm.h>

#include <bot/bot_core.h>
#include <bot/gtk/gtk_util.h>
#include <bot/gl/gl_util.h>

#include <bot/viewer/viewer.h>
#include <lcmtypes/navlcm_feature_list_t.h>
#include <lcmtypes/navlcm_flow_t.h>
#include<lcmtypes/navlcm_image_old_t.h>

#include <common/color_util.h>
#include <common/globals.h>
#include <common/navconf.h>
#include <common/texture.h>
#include <common/lcm_util.h>

#include "util.h"

#define MAX_IMAGES 3
#define MAX_FEATURE_SETS 3

#define RENDERER_NAME "Image"
#define PARAM_SHOW_INSTANT_FLOW "Instantaneous flow"
#define PARAM_SHOW_FEATURES "Features"
#define PARAM_SHOW_SENSOR_COLOR_BOX "Sensor color box"
#define PARAM_SHOW_DBG_FEATURES "Extra features"
#define PARAM_FIT_TO_WINDOW "Fit to window"
#define PARAM_SHOW_FEATURE_SCALE "Feature scales"
#define PARAM_COMPACT_LAYOUT "Compact layout"
#define PARAM_FLOW_SCALE "Flow scale"
#define PARAM_IMAGE_DESIRED_WIDTH "Image width"
#define PARAM_IMAGE_ORIGINAL_SIZE "Original size"
#define FLOW_SCALE_MIN .1
#define FLOW_SCALE_MAX 3.0
#define FLOW_SCALE_DF .1

typedef struct _RendererImage {
    Renderer renderer;

    BotConf *config;
    lcm_t *lcm;
    Viewer *viewer;
    BotGtkParamWidget *pw;
    GHashTable *img_channels;
    GQueue *features;
    char **channel_names;
    int nsensors;
    navlcm_feature_list_t *features_dbg;
    navlcm_flow_t *flow_instant;
    double param_flow_scale;
    int64_t utime;
    int image_width;
    int image_height;

} RendererImage;

static void my_free( Renderer *renderer )
{
    RendererImage *self = (RendererImage*) renderer->user;

    free( self );
}

static void my_draw( Viewer *viewer, Renderer *renderer )
{
    RendererImage *self = (RendererImage*) renderer->user;

    // transform into window coordinates, where <0, 0> is the top left corner
    // of the window and <viewport[2], viewport[3]> is the bottom right corner
    // of the window
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, viewport[2], 0, viewport[3]);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0, viewport[3], 0);
    glScalef (1, -1, 1);

    int desired_width = bot_gtk_param_widget_get_int (self->pw, PARAM_IMAGE_DESIRED_WIDTH);
    
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init (&iter, self->img_channels);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        char *channel = (char*)key;
        GQueue *q = (GQueue*)value;
        botlcm_image_t *img = (botlcm_image_t*)g_queue_peek_tail (q);

        int idx;
//        if (sscanf (channel, "CAM_THUMB_%d", &idx) != 1 && sscanf (channel, "OLDIMAGE_%d", &idx) != 1) {
        if (navconf_get_channel_index (self->channel_names, channel, &idx)<0) {
            printf ("Unexpected channel %s\n", channel);
        } else {

            util_draw_image (img, idx, viewport[2], viewport[3], 
                    bot_gtk_param_widget_get_bool(self->pw, PARAM_COMPACT_LAYOUT),
                    bot_gtk_param_widget_get_bool(self->pw, PARAM_FIT_TO_WINDOW), desired_width, self->nsensors, 0, 0, 10);

        }
    }

    // show features
    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_FEATURES)) { 
        gboolean show_scale = bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_FEATURE_SCALE);
        if (!g_queue_is_empty (self->features)) {
            navlcm_feature_list_t *s = (navlcm_feature_list_t*)g_queue_peek_tail (self->features);
            util_draw_features (s, viewport[2], viewport[3], 
                    bot_gtk_param_widget_get_bool(self->pw, PARAM_COMPACT_LAYOUT),
                    bot_gtk_param_widget_get_bool(self->pw, PARAM_FIT_TO_WINDOW), desired_width, self->nsensors, 0, 0, 10,
                    3, FALSE, FALSE, show_scale);
        }
    }

    // show dbg features
    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_DBG_FEATURES)) { 
        gboolean sensor_color_box = bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_SENSOR_COLOR_BOX); 
        if (self->features_dbg) {
            navlcm_feature_list_t *s = self->features_dbg;
            util_draw_features (s, viewport[2], viewport[3], 
                    bot_gtk_param_widget_get_bool(self->pw, PARAM_COMPACT_LAYOUT),
                    bot_gtk_param_widget_get_bool(self->pw, PARAM_FIT_TO_WINDOW), desired_width, self->nsensors, 0, 0, 10,
                    5, TRUE, sensor_color_box, FALSE);
        }
    }

    // show instantaneous flow
    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_INSTANT_FLOW)) { 
        self->param_flow_scale = bot_gtk_param_widget_get_double(self->pw, PARAM_FLOW_SCALE);
        if (self->flow_instant && self->image_width > 0)
            util_draw_flow (self->flow_instant, viewport[2], viewport[3], self->image_width, self->image_height, 
                    self->param_flow_scale, 
                    bot_gtk_param_widget_get_bool(self->pw, PARAM_COMPACT_LAYOUT),
                    bot_gtk_param_widget_get_bool(self->pw, PARAM_FIT_TO_WINDOW), desired_width, self->nsensors, 0, 0, 10, 2);
    }

    // show data time info
    if (self->image_width > 0) {
        time_t time = self->utime/1000000;
        double h = 1.0 * self->image_height * desired_width / self->image_width;
        double pos[3] = {10, h+10, 0};
        if (bot_gtk_param_widget_get_bool(self->pw, PARAM_COMPACT_LAYOUT))
            pos[1] += h;
        bot_gl_draw_text(pos, GLUT_BITMAP_HELVETICA_12, ctime (&time),
                BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
                BOT_GL_DRAW_TEXT_ANCHOR_TOP |
                BOT_GL_DRAW_TEXT_ANCHOR_LEFT |
                BOT_GL_DRAW_TEXT_DROP_SHADOW);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

}

static void on_param_widget_changed (BotGtkParamWidget *pw, const char *name, void *user)
{
    RendererImage *self = (RendererImage*) user;

    if (!strcmp (name, PARAM_IMAGE_ORIGINAL_SIZE)) {
        if (bot_gtk_param_widget_get_bool(self->pw, PARAM_IMAGE_ORIGINAL_SIZE)) 
            bot_gtk_param_widget_set_int (self->pw, PARAM_IMAGE_DESIRED_WIDTH, self->image_width);
    }

    viewer_request_redraw(self->viewer);
}

static void on_load_preferences (Viewer *viewer, GKeyFile *keyfile, void *user_data)
{
    RendererImage *self = (RendererImage*)user_data;
    bot_gtk_param_widget_load_from_key_file (self->pw, keyfile, RENDERER_NAME);
}

static void on_save_preferences (Viewer *viewer, GKeyFile *keyfile, void *user_data)
{
    RendererImage *self = (RendererImage*)user_data;
    bot_gtk_param_widget_save_to_key_file (self->pw, keyfile, RENDERER_NAME);
}

    static void
on_feature_list (const lcm_recv_buf_t *buf, const char *channel, const navlcm_feature_list_t *msg,
        void *user)
{
    RendererImage *self = (RendererImage*)user;

    g_queue_push_tail (self->features, navlcm_feature_list_t_copy (msg));

    if (g_queue_get_length (self->features) > MAX_FEATURE_SETS) {
        navlcm_feature_list_t *s = (navlcm_feature_list_t*)g_queue_pop_head (self->features);
        navlcm_feature_list_t_destroy (s);
    }

    self->utime = msg->utime;
    viewer_request_redraw(self->viewer);
}

    static void
on_flow_instant (const lcm_recv_buf_t *buf, const char *channel, const navlcm_flow_t *msg,
        void *user)
{
    RendererImage *self = (RendererImage*)user;

    // backup to local memory
    if (self->flow_instant)
        navlcm_flow_t_destroy (self->flow_instant);

    self->flow_instant = navlcm_flow_t_copy (msg);

    viewer_request_redraw(self->viewer);
}

    static void
on_features_dbg (const lcm_recv_buf_t *buf, const char *channel, const navlcm_feature_list_t *msg,
        void *user)
{
    RendererImage *self = (RendererImage*)user;

    // backup to local memory
    if (self->features_dbg)
        navlcm_feature_list_t_destroy (self->features_dbg);

    self->features_dbg = navlcm_feature_list_t_copy (msg);

    self->utime = msg->utime;
    viewer_request_redraw(self->viewer);
}

static void on_image (const lcm_recv_buf_t *rbuf, const char *channel, 
        const botlcm_image_t *msg, void *user_data )
{
    RendererImage *self = (RendererImage*) user_data;

    gpointer data = g_hash_table_lookup (self->img_channels, channel);
    GQueue *q = NULL;

    if (data) {
        q = (GQueue*)data;
    } else {
        q = g_queue_new ();
        g_hash_table_insert (self->img_channels, g_strdup (channel), q);
    }

    g_queue_push_tail (q, botlcm_image_t_copy (msg));

    if (g_queue_get_length (q) > MAX_IMAGES) {
        botlcm_image_t *img = (botlcm_image_t*)g_queue_pop_head (q);
        botlcm_image_t_destroy (img);
    }

    self->utime = msg->utime;
    self->image_width = msg->width;
    self->image_height = msg->height;

    viewer_request_redraw(self->viewer);
}

botlcm_image_t *my_navlcm_image_old_to_botlcm_image (const navlcm_image_old_t *im)
{
    botlcm_image_t *img = (botlcm_image_t*)calloc(1, sizeof(botlcm_image_t));

    img->utime = im->utime;
    img->width = im->width;
    img->height = im->height;
    img->row_stride = im->stride;
    img->pixelformat = im->pixelformat;
    img->size = im->size;
    img->data = (unsigned char*)malloc(im->size);
    memcpy (img->data, im->image, im->size);

    printf ("%d x %d = %d (size: %d)\n", im->width, im->height, im->width * im->height, im->size);
    // convert color to grayscale
    if (im->width * im->height * 3 == im->size) {
        unsigned char *data = (unsigned char*)calloc(im->width * im->height, 1);
        for (int i=0;i<im->width*im->height;i++) {
            double val = (1.0 * im->image[3*i] + 1.0 * im->image[3*i+1] + 1.0 * im->image[3*i+2])/3;
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

    return img;
}

static void on_old_image (const lcm_recv_buf_t *rbuf, const char *channel, 
        const navlcm_image_old_t *msg, void *user_data )
{
    RendererImage *self = (RendererImage*) user_data;

    gpointer data = g_hash_table_lookup (self->img_channels, channel);
    GQueue *q = NULL;

    if (data) {
        q = (GQueue*)data;
    } else {
        q = g_queue_new ();
        g_hash_table_insert (self->img_channels, g_strdup (channel), q);
    }

    g_queue_push_tail (q, my_navlcm_image_old_to_botlcm_image  (msg));

    if (g_queue_get_length (q) > MAX_IMAGES) {
        botlcm_image_t *img = (botlcm_image_t*)g_queue_pop_head (q);
        botlcm_image_t_destroy (img);
    }

    self->utime = msg->utime;
    self->image_width = msg->width;
    self->image_height = msg->height;

    viewer_request_redraw(self->viewer);
}

void setup_renderer_image(Viewer *viewer, int priority) 
{
    RendererImage *self = 
        (RendererImage*) calloc(1, sizeof(RendererImage));

    Renderer *renderer = &self->renderer;

    renderer->draw = my_draw;
    renderer->destroy = my_free;
    renderer->name = (char*)RENDERER_NAME;
    self->pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());

    renderer->widget = GTK_WIDGET(self->pw);
    renderer->enabled = 1;
    renderer->user = self;

    self->lcm = globals_get_lcm();
    self->config = globals_get_config();
    self->viewer = viewer;
    self->features = g_queue_new ();
    self->features_dbg = NULL;
    self->flow_instant = NULL;
    self->param_flow_scale = .4;
    self->utime = 0;
    self->image_width = 0;
    self->image_height = 0;

    navconf_get_nsensors (self->config, &self->nsensors);
    char *platform = NULL;
    assert (bot_conf_get_str (self->config, "platform", &platform)==0);
    char key[1024];
    sprintf (key, "cameras.%s.channels", platform);
    self->channel_names = bot_conf_get_str_array_alloc(self->config, key);
    if (!self->channel_names) {
        fprintf (stderr, "failed to read channel names.\n");
        return;
    }

    self->img_channels = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_SHOW_FEATURES, 1, PARAM_SHOW_DBG_FEATURES, 0, PARAM_SHOW_FEATURE_SCALE, 0, NULL);

    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_SHOW_SENSOR_COLOR_BOX, 1, NULL);

    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_SHOW_INSTANT_FLOW, 1, NULL);

    bot_gtk_param_widget_add_double(self->pw, PARAM_FLOW_SCALE,
            BOT_GTK_PARAM_WIDGET_SLIDER,
            FLOW_SCALE_MIN, FLOW_SCALE_MAX, FLOW_SCALE_DF,
            self->param_flow_scale);

    bot_gtk_param_widget_add_double(self->pw, PARAM_IMAGE_DESIRED_WIDTH,
            BOT_GTK_PARAM_WIDGET_SLIDER, 1, 752, 10, 376);

    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_IMAGE_ORIGINAL_SIZE, 0, PARAM_FIT_TO_WINDOW, 1, PARAM_COMPACT_LAYOUT, 1, NULL);

    g_signal_connect (G_OBJECT (self->pw), "changed", 
            G_CALLBACK (on_param_widget_changed), self);

    viewer_add_renderer(viewer, renderer, priority);

    botlcm_image_t_subscribe (self->lcm, "CAM_THUMB.*", on_image, self);
    navlcm_image_old_t_subscribe (self->lcm, "OLDIMAGE.*", on_old_image, self);
    navlcm_feature_list_t_subscribe (self->lcm, "FEATURE_SET", on_feature_list, self);
    navlcm_feature_list_t_subscribe (self->lcm, "FEATURES_DBG", on_features_dbg, self);
    navlcm_flow_t_subscribe (self->lcm, "FLOW_INSTANT", on_flow_instant, self);

    g_signal_connect (G_OBJECT (viewer), "load-preferences", 
            G_CALLBACK (on_load_preferences), self);
    g_signal_connect (G_OBJECT (viewer), "save-preferences",
            G_CALLBACK (on_save_preferences), self);

}
