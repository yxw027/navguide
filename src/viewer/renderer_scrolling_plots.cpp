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

#include <lcmtypes/navlcm_class_param_t.h>

#define PARAM_NAME_GRAPH_TIMESPAN "Time span"
#define PARAM_NAME_FREEZE "Freeze"
#define PARAM_NAME_SIZE "Size"
#define PARAM_NAME_RENDER_PSI_DISTANCE "Psi distance"
#define PARAM_NAME_SHOW_LEGEND "Show Legends"

#define REDRAW_THRESHOLD_UTIME 5000000

typedef struct _RendererScrollingPlots RendererScrollingPlots;

struct _RendererScrollingPlots {
    Renderer renderer;
    Viewer *viewer;
    lcm_t *lcm;

    BotGtkParamWidget    *pw;

    BotGlScrollPlot2d *psi_distance_plot;
    
    uint64_t      max_utime;
};

static void on_param_widget_changed (BotGtkParamWidget *pw, const char *name, 
        RendererScrollingPlots *self);
static void 
update_xaxis (RendererScrollingPlots *self, uint64_t utime);
static void
on_class_state (const lcm_recv_buf_t *buf, const char *channel, const navlcm_class_param_t *msg, void *user);

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

    bot_gl_scrollplot2d_set_xlim (self->psi_distance_plot, gs_ts_min, gs_ts_max);

    int plot_width = bot_gtk_param_widget_get_int (self->pw, PARAM_NAME_SIZE);
    int plot_height = plot_width / 2;

    int x = viewport[2] - plot_width  - 10;
    int y = viewport[1] +10 ;

    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_RENDER_PSI_DISTANCE)) {
        bot_gl_scrollplot2d_gl_render_at_window_pos (self->psi_distance_plot, 
                x, y, plot_width, plot_height);
        y += plot_height;
    }
}

static void
scrolling_plots_free (Renderer *renderer) 
{
    RendererScrollingPlots *self = (RendererScrollingPlots*) renderer;
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
    self->renderer.name = (char*)"Scrolling Plots";
    self->renderer.user = self;
    self->renderer.enabled = 1;

    self->renderer.widget = gtk_alignment_new (0, 0.5, 1.0, 0);

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
    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint) 0,
                                       PARAM_NAME_RENDER_PSI_DISTANCE, 1, NULL);
    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_NAME_SHOW_LEGEND, 0, NULL);

    g_signal_connect (G_OBJECT (self->pw), "changed", 
            G_CALLBACK (on_param_widget_changed), self);

    
    // psi_distance plot
    self->psi_distance_plot = bot_gl_scrollplot2d_new ();
    bot_gl_scrollplot2d_set_title        (self->psi_distance_plot, "Psi distance");
    bot_gl_scrollplot2d_set_text_color   (self->psi_distance_plot, 0.7, 0.7, 0.7, 1);
    bot_gl_scrollplot2d_set_bgcolor      (self->psi_distance_plot, 0.1, 0.1, 0.1, 0.7);
    bot_gl_scrollplot2d_set_border_color (self->psi_distance_plot, 1, 1, 1, 0.7);
    bot_gl_scrollplot2d_set_ylim    (self->psi_distance_plot, 0.2, 1);
    bot_gl_scrollplot2d_add_plot    (self->psi_distance_plot, "control", 1000);
    bot_gl_scrollplot2d_set_color   (self->psi_distance_plot, "control", 0, 0, 1, 1);
    bot_gl_scrollplot2d_add_plot    (self->psi_distance_plot, "status", 1000);
    bot_gl_scrollplot2d_set_color   (self->psi_distance_plot, "status", 1, 0, 0, 1);

    // legends?
    BotGlScrollPlot2dLegendLocation legloc = BOT_GL_SCROLLPLOT2D_HIDDEN;
    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_SHOW_LEGEND)) {
        legloc = BOT_GL_SCROLLPLOT2D_TOP_RIGHT;
    }
    bot_gl_scrollplot2d_set_show_legend (self->psi_distance_plot, legloc);

    // subscribe to LCM messages
    navlcm_class_param_t_subscribe (self->lcm, "CLASS_STATE", on_class_state, self);

    return &self->renderer;
}

void setup_renderer_scrolling_plots (Viewer *viewer, int render_priority)
{
    viewer_add_renderer(viewer, 
            renderer_scrolling_plots_new(viewer), render_priority);
}

static void
on_class_state (const lcm_recv_buf_t *buf, const char *channel, const navlcm_class_param_t *msg, void *user)
{
    RendererScrollingPlots *self = (RendererScrollingPlots *) user;

    if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_FREEZE))
        return;

    update_xaxis(self, msg->utime);

    double timestamp = msg->utime * 1e-6;

    bot_gl_scrollplot2d_add_point (self->psi_distance_plot, "status", timestamp, 
                              msg->psi_distance);
    bot_gl_scrollplot2d_add_point (self->psi_distance_plot, "control", timestamp, 
                              msg->psi_distance_thresh);

    viewer_request_redraw (self->viewer);
}

static void 
on_param_widget_changed (BotGtkParamWidget *pw, const char *name, 
        RendererScrollingPlots *self)
{
    if (! strcmp (name, PARAM_NAME_SHOW_LEGEND)) {
        BotGlScrollPlot2dLegendLocation legloc = BOT_GL_SCROLLPLOT2D_HIDDEN;
        if (bot_gtk_param_widget_get_bool (self->pw, PARAM_NAME_SHOW_LEGEND)) {
            legloc = BOT_GL_SCROLLPLOT2D_TOP_RIGHT;
        }
        bot_gl_scrollplot2d_set_show_legend (self->psi_distance_plot, legloc);
    }
    viewer_request_redraw (self->viewer);
}

static void 
update_xaxis (RendererScrollingPlots *self, uint64_t utime)
{
    if ((utime < self->max_utime) && 
        (utime > self->max_utime - REDRAW_THRESHOLD_UTIME)) return;
    self->max_utime = utime;
}

