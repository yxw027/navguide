/*
 * Renders a set of scrolling plots in the top right corner of the window
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <gdk/gdkkeysyms.h>

#include <bot/bot_core.h>

#include <bot/gtk/gtk_util.h>
#include <bot/viewer/viewer.h>
#include <bot/gl/gl_util.h>
#include <bot/gl/scrollplot2d.h>

#include <common/globals.h>
#include <common/math_util.h>

#include <lcmtypes/fblcm_motion_cmd_t.h>

#define PARAM_NAME_GRAPH_TIMESPAN "Time span"
#define PARAM_NAME_FREEZE "Freeze"
#define PARAM_NAME_SIZE "Size"
#define PARAM_NAME_RENDER_NAVIGATOR_ROTATION "Navigator Rotation"
#define PARAM_NAME_RENDER_NAVIGATOR_TRANSLATION "Navigator Translation"
#define PARAM_NAME_RENDER_ENCODER_LEFT "Left Encoder"
#define PARAM_NAME_RENDER_ENCODER_RIGHT "Right Encoder"

#define PARAM_NAME_SHOW_LEGEND "Show Legends"

#define REDRAW_THRESHOLD_UTIME 5000000

typedef struct _RendererScrollingPlots RendererScrollingPlots;

struct _RendererScrollingPlots {
    Renderer renderer;
    Viewer *viewer;
    lcm_t *lcm;
    ATrans *atrans;

    BotGtkParamWidget    *pw;

    BotGlScrollPlot2d *navigator_rotation_plot;
    BotGlScrollPlot2d *navigator_translation_plot;
    BotGlScrollPlot2d *encoder_left_plot;
    BotGlScrollPlot2d *encoder_right_plot;
    
    int32_t encoder_left_last;
    int64_t encoder_left_last_utime;
    int32_t encoder_right_last;
    int64_t encoder_right_last_utime;
    double encoder_left_scale;
    double encoder_right_scale;

    uint64_t      max_utime;
};

static void on_motion_cmd (const lcm_recv_buf_t *buf, const char *channel,
                                          const fblcm_motion_cmd_t *msg, void *user);

static void update_xaxis (RendererScrollingPlots *self, uint64_t utime);

static void on_param_widget_changed (BotGtkParamWidget *pw, const char *name, 
        RendererScrollingPlots *self);

static void
scrolling_plots_draw (Viewer *viewer, Renderer *renderer)
{
    RendererScrollingPlots *self = (RendererScrollingPlots*) renderer->user;
    if (!self->max_utime) return;

    GLdouble model_matrix[16];
    GLdouble proj_matrix[16];
    GLint viewport[4];

    glGetDoublev (GL_MODELVIEW_MATRIX, model_matrix);
    glGetDoublev (GL_PROJECTION_MATRIX, proj_matrix);
    glGetIntegerv (GL_VIEWPORT, viewport);

    double gs_ts_max = self->max_utime * 1e-6;
    double gs_ts_min = gs_ts_max - 
        bot_gtk_param_widget_get_double (self->pw, PARAM_NAME_GRAPH_TIMESPAN);

    bot_gl_scrollplot2d_set_xlim (self->navigator_rotation_plot, gs_ts_min, gs_ts_max);
    bot_gl_scrollplot2d_set_xlim (self->navigator_translation_plot, gs_ts_min, gs_ts_max);
    bot_gl_scrollplot2d_set_xlim (self->encoder_left_plot, gs_ts_min, gs_ts_max);
    bot_gl_scrollplot2d_set_xlim (self->encoder_right_plot, gs_ts_min, gs_ts_max);

    int plot_width = bot_gtk_param_widget_get_int (self->pw, PARAM_NAME_SIZE);
    int plot_height = plot_width / 3;

    int x = viewport[2] - plot_width;
    int y = viewport[1];

    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_RENDER_NAVIGATOR_ROTATION)) {
        bot_gl_scrollplot2d_gl_render_at_window_pos (self->navigator_rotation_plot, 
                x, y, plot_width, plot_height);
        y += plot_height;
    }

    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_RENDER_NAVIGATOR_TRANSLATION)) {
        bot_gl_scrollplot2d_gl_render_at_window_pos (self->navigator_translation_plot, 
                x, y, plot_width, plot_height);
        y += plot_height;
    }

    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_RENDER_ENCODER_LEFT)) {
        bot_gl_scrollplot2d_gl_render_at_window_pos (self->encoder_left_plot, 
                                                     x, y, plot_width, plot_height);
        y += plot_height;
    }

    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_RENDER_ENCODER_RIGHT)) {
        bot_gl_scrollplot2d_gl_render_at_window_pos (self->encoder_right_plot, 
                                                     x, y, plot_width, plot_height);
        y += plot_height;
    }

}

static void
scrolling_plots_free (Renderer *renderer) 
{
    RendererScrollingPlots *self = (RendererScrollingPlots*) renderer;
    globals_release_atrans (self->atrans);
    globals_release_lcm (self->lcm);
    free (renderer);
}

Renderer *renderer_scrolling_plots_new (Viewer *viewer)
{
    RendererScrollingPlots *self = 
        (RendererScrollingPlots*) calloc (1, sizeof (RendererScrollingPlots));
    self->viewer = viewer;
    self->renderer.draw = scrolling_plots_draw;
    self->renderer.destroy = scrolling_plots_free;
    self->renderer.name = "Scrolling Plots";
    self->renderer.user = self;
    self->renderer.enabled = 1;

    self->renderer.widget = gtk_alignment_new (0, 0.5, 1.0, 0);
    self->atrans = globals_get_atrans();

    self->lcm = globals_get_lcm ();

    self->pw = BOT_GTK_PARAM_WIDGET (bot_gtk_param_widget_new ());
    gtk_container_add (GTK_CONTAINER (self->renderer.widget), 
            GTK_WIDGET(self->pw));
    gtk_widget_show (GTK_WIDGET (self->pw));

    bot_gtk_param_widget_add_int (self->pw, PARAM_NAME_SIZE,
            BOT_GTK_PARAM_WIDGET_SLIDER, 50, 800, 10, 150);
    bot_gtk_param_widget_add_double (self->pw, PARAM_NAME_GRAPH_TIMESPAN, 
            BOT_GTK_PARAM_WIDGET_SLIDER, 1, 20, 0.5, 5);
    bot_gtk_param_widget_add_booleans (self->pw, 
            BOT_GTK_PARAM_WIDGET_TOGGLE_BUTTON, PARAM_NAME_FREEZE, 0, NULL);
    bot_gtk_param_widget_add_booleans (self->pw, 0,
                                       PARAM_NAME_RENDER_NAVIGATOR_ROTATION, 1, 
                                       PARAM_NAME_RENDER_NAVIGATOR_TRANSLATION, 1, 
                                       NULL);
    bot_gtk_param_widget_add_booleans (self->pw, 0,
                                       PARAM_NAME_RENDER_ENCODER_LEFT, 1,
                                       PARAM_NAME_RENDER_ENCODER_RIGHT, 1, NULL);
    bot_gtk_param_widget_add_booleans (self->pw, 0, 
            PARAM_NAME_SHOW_LEGEND, 0, NULL);

    g_signal_connect (G_OBJECT (self->pw), "changed", 
            G_CALLBACK (on_param_widget_changed), self);

    
    BotConf *config = globals_get_config ();
    self->encoder_left_scale = 1.0;//bot_conf_get_double_or_fail (config, "calibration.encoders.WHEEL_LEFT.scale");
    self->encoder_right_scale = 1.0;//bot_conf_get_double_or_fail (config, "calibration.encoders.WHEEL_RIGHT.scale");
    globals_release_config (config);

    self->encoder_left_last = 0;
    self->encoder_right_last = 0;

    // navigator rotation
    self->navigator_rotation_plot = bot_gl_scrollplot2d_new ();
    bot_gl_scrollplot2d_set_title        (self->navigator_rotation_plot, "Nav. Rot. Speed");
    bot_gl_scrollplot2d_set_text_color   (self->navigator_rotation_plot, 0.7, 0.7, 0.7, 1);
    bot_gl_scrollplot2d_set_bgcolor      (self->navigator_rotation_plot, 0.1, 0.1, 0.1, 0.7);
    bot_gl_scrollplot2d_set_border_color (self->navigator_rotation_plot, 1, 1, 1, 0.7);
    bot_gl_scrollplot2d_set_ylim    (self->navigator_rotation_plot, -.5, .5);
    bot_gl_scrollplot2d_add_plot    (self->navigator_rotation_plot, "control", 1000);
    bot_gl_scrollplot2d_set_color   (self->navigator_rotation_plot, "control", 0.7, 0, 0.7, 1);

    bot_gl_scrollplot2d_add_plot    (self->navigator_rotation_plot, "status", 1000);
    bot_gl_scrollplot2d_set_color   (self->navigator_rotation_plot, "status", 0, 0, 1, 1);

    //bot_gl_scrollplot2d_add_plot    (self->navigator_rotation_plot, "2500", 1000);
    //bot_gl_scrollplot2d_set_color   (self->navigator_rotation_plot, "2500", 0.8, 0.8, 0.8, 0.5);


    self->navigator_translation_plot = bot_gl_scrollplot2d_new ();
    bot_gl_scrollplot2d_set_title        (self->navigator_translation_plot, "Nav. Trans. Speed");
    bot_gl_scrollplot2d_set_text_color   (self->navigator_translation_plot, 0.7, 0.7, 0.7, 1);
    bot_gl_scrollplot2d_set_bgcolor      (self->navigator_translation_plot, 0.1, 0.1, 0.1, 0.7);
    bot_gl_scrollplot2d_set_border_color (self->navigator_translation_plot, 1, 1, 1, 0.7);
    bot_gl_scrollplot2d_set_ylim    (self->navigator_translation_plot, 0, 2.0);
    bot_gl_scrollplot2d_add_plot    (self->navigator_translation_plot, "control", 1000);
    bot_gl_scrollplot2d_set_color   (self->navigator_translation_plot, "control", 0.7, 0, 0.7, 1);

    bot_gl_scrollplot2d_add_plot    (self->navigator_translation_plot, "status", 1000);
    bot_gl_scrollplot2d_set_color   (self->navigator_translation_plot, "status", 0, 0, 1, 1);

    // left wheel encoder plot
    self->encoder_left_plot = bot_gl_scrollplot2d_new ();
    bot_gl_scrollplot2d_set_title        (self->encoder_left_plot, "Left Encoder");
    bot_gl_scrollplot2d_set_text_color   (self->encoder_left_plot, 0.7, 0.7, 0.7, 1);
    bot_gl_scrollplot2d_set_border_color (self->encoder_left_plot, 1, 1, 1, 0.7);
    bot_gl_scrollplot2d_set_bgcolor (self->encoder_left_plot, 0.1, 0.1, 0.1, 0.7);
    bot_gl_scrollplot2d_set_ylim    (self->encoder_left_plot, 0, 2.0);

    bot_gl_scrollplot2d_add_plot    (self->encoder_left_plot, "actual", 1000);
    bot_gl_scrollplot2d_set_color   (self->encoder_left_plot, "actual", 0.7, 0, 0.7, 1);

    bot_gl_scrollplot2d_add_plot    (self->encoder_left_plot, "0", 1000);
    bot_gl_scrollplot2d_set_color   (self->encoder_left_plot, "0", 0.8, 0.8, 0.8, 0.5);

    // right wheel encoder plot
    self->encoder_right_plot = bot_gl_scrollplot2d_new ();
    bot_gl_scrollplot2d_set_title        (self->encoder_right_plot, "Right Encoder");
    bot_gl_scrollplot2d_set_text_color   (self->encoder_right_plot, 0.7, 0.7, 0.7, 1);
    bot_gl_scrollplot2d_set_border_color (self->encoder_right_plot, 1, 1, 1, 0.7);
    bot_gl_scrollplot2d_set_bgcolor (self->encoder_right_plot, 0.1, 0.1, 0.1, 0.7);
    bot_gl_scrollplot2d_set_ylim    (self->encoder_right_plot, 0, 2.0);

    bot_gl_scrollplot2d_add_plot    (self->encoder_right_plot, "actual", 1000);
    bot_gl_scrollplot2d_set_color   (self->encoder_right_plot, "actual", 0.7, 0, 0.7, 1);

    bot_gl_scrollplot2d_add_plot    (self->encoder_right_plot, "0", 1000);
    bot_gl_scrollplot2d_set_color   (self->encoder_right_plot, "0", 0.8, 0.8, 0.8, 0.5);

    // legends?
    BotGlScrollPlot2dLegendLocation legloc = BOT_GL_SCROLLPLOT2D_HIDDEN;
    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_SHOW_LEGEND)) {
        legloc = BOT_GL_SCROLLPLOT2D_TOP_RIGHT;
    }
    bot_gl_scrollplot2d_set_show_legend (self->navigator_rotation_plot, legloc);
    bot_gl_scrollplot2d_set_show_legend (self->navigator_translation_plot, legloc);

    // subscribe to LC messages
    fblcm_motion_cmd_t_subscribe (self->lcm, "MOTION_CMD", on_motion_cmd, self);
    
    //arlcm_encoder_t_subscribe (self->lcm, "WHEEL_LEFT", on_encoders, self);
    //arlcm_encoder_t_subscribe (self->lcm, "WHEEL_RIGHT", on_encoders, self);

    // periodically pull pose data from ATrans
    //g_timeout_add (30, get_speed_update, self);

    return &self->renderer;
}

void setup_renderer_scrolling_plots (Viewer *viewer, int render_priority)
{
    viewer_add_renderer(viewer, 
            renderer_scrolling_plots_new(viewer), render_priority);
}

static void
on_motion_cmd (const lcm_recv_buf_t *buf, const char *channel, const fblcm_motion_cmd_t *msg, void *user)
{
    RendererScrollingPlots *self = (RendererScrollingPlots *) user;

    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_FREEZE))
        return;

    update_xaxis(self, msg->utime);

    double timestamp = msg->utime * 1e-6;

    bot_gl_scrollplot2d_add_point (self->navigator_rotation_plot, "control", timestamp, 
                              msg->rotation_speed);
    bot_gl_scrollplot2d_add_point (self->navigator_translation_plot, "control", timestamp, 
                              msg->translation_speed);

    viewer_request_redraw (self->viewer);
}

/*
   static void
   on_encoders (const lcm_recv_buf_t * buf, const char *channel, const arlcm_encoder_t *msg, 
   void *user_data)
   {
   RendererScrollingPlots *self = (RendererScrollingPlots*) user_data;

   update_xaxis(self,msg->utime);
   if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_FREEZE)) return;

   if (!strcmp (channel, "WHEEL_LEFT")) {
   double diff = (double)msg->encoder - (double)self->encoder_left_last;
   double dt = ((double) (msg->utime - self->encoder_left_last_utime))*1e-6;
   double speed = fabs (self->encoder_left_scale * diff/dt);

   bot_gl_scrollplot2d_add_point (self->encoder_left_plot, "actual", 
   msg->utime * 1.0e-6,
   speed);


   self->encoder_left_last = msg->encoder;
   self->encoder_left_last_utime = msg->utime;

   }
   else if (!strcmp (channel, "WHEEL_RIGHT")) {
   double diff = (double)msg->encoder - (double)self->encoder_right_last;
   double dt = ((double) (msg->utime - self->encoder_right_last_utime))*1e-6;
   double speed = fabs (self->encoder_right_scale * diff/dt);

   bot_gl_scrollplot2d_add_point (self->encoder_right_plot, "actual", 
   msg->utime * 1.0e-6,
   speed);

   self->encoder_right_last = msg->encoder;
   self->encoder_right_last_utime = msg->utime;

   }

   return;

   }
   */


    static void 
on_param_widget_changed (BotGtkParamWidget *pw, const char *name, 
        RendererScrollingPlots *self)
{
    if (! strcmp (name, PARAM_NAME_SHOW_LEGEND)) {
        BotGlScrollPlot2dLegendLocation legloc = BOT_GL_SCROLLPLOT2D_HIDDEN;
        if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_SHOW_LEGEND)) {
            legloc = BOT_GL_SCROLLPLOT2D_TOP_RIGHT;
        }
        bot_gl_scrollplot2d_set_show_legend (self->navigator_rotation_plot, legloc);
        bot_gl_scrollplot2d_set_show_legend (self->navigator_translation_plot, legloc);
    }
    viewer_request_redraw (self->viewer);
}

    static void 
update_xaxis (RendererScrollingPlots *self, uint64_t utime)
{
    if ((utime < self->max_utime) && 
            (utime > self->max_utime - REDRAW_THRESHOLD_UTIME)) return;

    self->max_utime = utime;
    //double timestamp = self->max_utime * 1e-6;
    //bot_gl_scrollplot2d_add_point (self->steer_plot, "0", timestamp, 0.0);
}

/*
   static gboolean
   get_speed_update (void *user_data)
   {
   RendererScrollingPlots *self = (RendererScrollingPlots*) user_data;
   if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_FREEZE)) return 0;

   botlcm_pose_t pose;
   if(!atrans_get_local_pose(self->atrans, &pose))
   return TRUE;

   double speed = sqrt(sq(pose.vel[0]) + sq(pose.vel[1]) + sq(pose.vel[2]));

   bot_gl_scrollplot2d_add_point (self->speed_plot, "pose", 
   pose.utime * 1e-6, speed);
   return TRUE;
   }
   */
