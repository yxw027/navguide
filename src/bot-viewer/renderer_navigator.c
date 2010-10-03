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

#include <common/color_util.h>
#include <common/globals.h>
#include <common/fbconf.h>

#include <lcmtypes/fblcm_motion_cmd_t.h>

#define RENDERER_NAME "Navigator"

typedef struct _RendererController {
    Renderer renderer;

    BotConf *config;
    lcm_t *lcm;
    Viewer *viewer;
    BotGtkParamWidget *pw;

    fblcm_motion_cmd_t *motion_cmd;

} RendererController;

static void my_free( Renderer *renderer )
{
    RendererController *self = (RendererController*) renderer->user;

    free( self );
}

static void my_draw( Viewer *viewer, Renderer *renderer )
{
    RendererController *self = (RendererController*) renderer->user;

    if (!self->motion_cmd)
        return;

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
       
    /* draw rotation speed */
    glLineWidth(1);
    glColor3f(1,1,1);
    double center[2] = {40,50};
    double radius = 20;
    glBegin (GL_LINE_LOOP);
    for (int i=0;i<90;i++) {
        glVertex2f (center[0]+radius*cos(2*i/M_PI), center[1]+radius*sin(2*i/M_PI));
    }
    glEnd ();

    double max_rotation_speed = .2;
    double max_translation_speed = 1.0;

    glLineWidth(2);
    glColor3f (1,0,0);
    glBegin (GL_LINES);
    glVertex2f (center[0], center[1]);
    glVertex2f (center[0]+radius*cos(self->motion_cmd->rotation_speed/max_rotation_speed*(M_PI/2)+M_PI/2), 
            center[1]-radius*sin(self->motion_cmd->rotation_speed/max_rotation_speed*M_PI/2+M_PI/2));
    glEnd ();

    /* draw translation speed */
    glColor3f (.05,.05,.05);
    double qcenter[2] = {100, 50};
    glBegin (GL_QUADS);
    glVertex2f (qcenter[0]-radius/2, qcenter[1]-radius);
    glVertex2f (qcenter[0]+radius/2, qcenter[1]-radius);
    glVertex2f (qcenter[0]+radius/2, qcenter[1]+radius);
    glVertex2f (qcenter[0]-radius/2, qcenter[1]+radius);
    glEnd ();
    glColor3f(.7,0,0);
    glBegin (GL_QUADS);
    double delta = 2.0 * radius * (self->motion_cmd->translation_speed - max_translation_speed/2)/max_translation_speed;
    glVertex2f (qcenter[0]-radius/2, qcenter[1]-delta);
    glVertex2f (qcenter[0]+radius/2, qcenter[1]-delta);
    glVertex2f (qcenter[0]+radius/2, qcenter[1]+radius);
    glVertex2f (qcenter[0]-radius/2, qcenter[1]+radius);
    glEnd ();

    double tpos[3] = {30,0,100};
    glColor3f(1,1,1);
    bot_gl_draw_text(tpos, GLUT_BITMAP_HELVETICA_12, "navigator",
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT |
            BOT_GL_DRAW_TEXT_DROP_SHADOW);

    glColor3f (1,1,1);
    glBegin (GL_LINES);
    glVertex2f (10, 20);
    glVertex2f (120, 20);
    glEnd ();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

}

static void on_motion_cmd (const lcm_recv_buf_t *rbuf, const char *channel, 
        const fblcm_motion_cmd_t *msg, void *user )
{
    RendererController *self = (RendererController*) user;

    if (self->motion_cmd)
        fblcm_motion_cmd_t_destroy (self->motion_cmd);

    self->motion_cmd = fblcm_motion_cmd_t_copy (msg);

    viewer_request_redraw(self->viewer);
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

void setup_renderer_navigator(Viewer *viewer, int priority) 
{
    RendererController *self = 
        (RendererController*) calloc(1, sizeof(RendererController));

    Renderer *renderer = &self->renderer;

    renderer->draw = my_draw;
    renderer->destroy = my_free;
    renderer->name = RENDERER_NAME;
    self->pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());
    self->motion_cmd = NULL;

    renderer->widget = GTK_WIDGET(self->pw);
    renderer->enabled = 1;
    renderer->user = self;

    self->lcm = globals_get_lcm();
    self->config = globals_get_config();
    self->viewer = viewer;

    fblcm_motion_cmd_t_subscribe(self->lcm, "MOTION_CMD", on_motion_cmd, self);

    g_signal_connect (G_OBJECT (self->pw), "changed", 
            G_CALLBACK (on_param_widget_changed), self);

    viewer_add_renderer(viewer, renderer, priority);

    g_signal_connect (G_OBJECT (viewer), "load-preferences", 
            G_CALLBACK (on_load_preferences), self);
    g_signal_connect (G_OBJECT (viewer), "save-preferences",
            G_CALLBACK (on_save_preferences), self);

}
