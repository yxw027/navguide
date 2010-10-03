#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <lcm/lcm.h>

#include <bot/bot_core.h>
#include <bot/gtk/gtk_util.h>
#include <bot/viewer/viewer.h>

#include <common/color_util.h>
#include <common/globals.h>
#include <common/fbconf.h>

#define RENDERER_NAME "Controller"

typedef struct _RendererController {
    Renderer renderer;

    BotConf *config;
    lcm_t *lcm;
    Viewer *viewer;
    BotGtkParamWidget *pw;

} RendererController;

static void my_free( Renderer *renderer )
{
    RendererController *self = (RendererController*) renderer->user;

    free( self );
}

static void my_draw( Viewer *viewer, Renderer *renderer )
{
    RendererController *self = (RendererController*) renderer->user;

}

static void on_param_widget_changed (BotGtkParamWidget *pw, const char *name, void *user)
{
    RendererController *self = (RendererController*) user;
    viewer_request_redraw(self->viewer);
}

static void on_load_preferences (Viewer *viewer, GKeyFile *keyfile, void *user_data)
{
    RendererController *self = user_data;
    bot_gtk_param_widget_load_from_key_file (self->pw, keyfile, RENDERER_NAME);
}

static void on_save_preferences (Viewer *viewer, GKeyFile *keyfile, void *user_data)
{
    RendererController *self = user_data;
    bot_gtk_param_widget_save_to_key_file (self->pw, keyfile, RENDERER_NAME);
}

void setup_renderer_controller(Viewer *viewer, int priority) 
{
    RendererController *self = 
        (RendererController*) calloc(1, sizeof(RendererController));

    Renderer *renderer = &self->renderer;

    renderer->draw = my_draw;
    renderer->destroy = my_free;
    renderer->name = RENDERER_NAME;
    self->pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());
    renderer->widget = GTK_WIDGET(self->pw);
    renderer->enabled = 1;
    renderer->user = self;

    self->lcm = globals_get_lcm();
    self->config = globals_get_config();
    self->viewer = viewer;

    g_signal_connect (G_OBJECT (self->pw), "changed", 
                      G_CALLBACK (on_param_widget_changed), self);
    
    viewer_add_renderer(viewer, renderer, priority);

    g_signal_connect (G_OBJECT (viewer), "load-preferences", 
            G_CALLBACK (on_load_preferences), self);
    g_signal_connect (G_OBJECT (viewer), "save-preferences",
            G_CALLBACK (on_save_preferences), self);

}
