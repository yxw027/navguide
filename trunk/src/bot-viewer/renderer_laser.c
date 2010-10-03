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

#define RENDERER_NAME "Laser"
#define PARAM_MEMORY "Scan memory"
#define PARAM_SPATIAL_DECIMATE "Spatial Decimation"
#define PARAM_BIG_POINTS "Big points"

#define COLOR_MENU "Color"
enum {
    COLOR_DRAB,
    COLOR_INTENSITY,
    COLOR_LASER,
};

struct laser_data
{
    botlcm_planar_lidar_t  *msg;
    BotTrans to_local;
    double to_local_mat[12];
//    double   m[16];
};

struct laser_channel
{
    char      name[256];

    BotPtrCircular *data;

    double    color[3];
//    char      calib[256];
    int       complained;
};

typedef struct _RendererLaser {
    Renderer renderer;

    BotConf *config;
    lcm_t *lcm;
    GHashTable *channels_hashtable;
    GPtrArray  *channels;
    Viewer *viewer;
    BotGtkParamWidget *pw;

    ATrans * atrans;
    int64_t last_utime;
} RendererLaser;

static void free_ldata(void *user, void *p)
{
    struct laser_data *ldata = (struct laser_data*) p;
    botlcm_planar_lidar_t_destroy(ldata->msg);
    free(ldata);
}

static void my_free( Renderer *renderer )
{
    RendererLaser *self = (RendererLaser*) renderer->user;

    free( self );
}

static void my_draw( Viewer *viewer, Renderer *renderer )
{
    RendererLaser *self = (RendererLaser*) renderer->user;

    int colormode = bot_gtk_param_widget_get_enum(self->pw, COLOR_MENU);
    
    for (unsigned int chanidx = 0; chanidx < bot_g_ptr_array_size(self->channels); chanidx++) {
        struct laser_channel *lchannel = g_ptr_array_index(self->channels, chanidx);

        if (bot_gtk_param_widget_get_bool(self->pw, PARAM_BIG_POINTS))
            glPointSize(8.0f);
        else
            glPointSize(4.0f);

        glBegin(GL_POINTS);
        
        for (unsigned int didx = 0; didx < bot_ptr_circular_size(lchannel->data); didx++) {
            struct laser_data *ldata = (struct laser_data*) bot_ptr_circular_index(lchannel->data, didx);

            for (int rangeidx = 0; rangeidx < ldata->msg->nranges; rangeidx++) {
                double range = ldata->msg->ranges[rangeidx];
                double theta = ldata->msg->rad0 + ldata->msg->radstep*rangeidx;

                switch(colormode) 
                {
                case COLOR_INTENSITY:
                    if (ldata->msg->nintensities == ldata->msg->nranges) {
                        double v = ldata->msg->intensities[rangeidx];
                        if (v > 1) {
                            // old non-normalized encoding
                            double minv = 7000, maxv = 12000;
                            v = (v-minv)/(maxv-minv);
                        }
                        glColor3fv(color_util_jet(v));
                        break;
                    }
                    // no intensity data? fall through!
                case COLOR_DRAB:
                    glColor3d(0.3, 0.3, 0.3);
                    break;
                case COLOR_LASER:
                    glColor3d(lchannel->color[0], lchannel->color[1], lchannel->color[2]);
                    break;
                }
                
                if (range > 79.0)
                    continue;
                
                double sensor_xyz[3], local_xyz[3];
                double s, c;
                bot_fasttrig_sincos(theta, &s, &c);
                
                sensor_xyz[0] = c * range;
                sensor_xyz[1] = s * range;
                sensor_xyz[2] = 0;
                
                bot_vector_affine_transform_3x4_3d(ldata->to_local_mat, sensor_xyz, local_xyz);
                glVertex3dv(local_xyz);
            }
        }

        glEnd();
    }
}

static void on_laser (const lcm_recv_buf_t *rbuf, const char *channel, 
        const botlcm_planar_lidar_t *msg, void *user_data )
{
    RendererLaser *self = (RendererLaser*) user_data;

    if(!atrans_have_trans(self->atrans, "body", "local"))
        return;

    double dt = (msg->utime - self->last_utime)/1000000.0;
    self->last_utime = msg->utime;
    if (fabs(dt) > 3) {
        for (unsigned int i = 0; i < bot_g_ptr_array_size(self->channels); i++) {
            struct laser_channel *lchan = (struct laser_channel*) g_ptr_array_index(self->channels, i);
            bot_ptr_circular_clear(lchan->data);
        }
    }

    struct laser_channel *lchannel = g_hash_table_lookup(self->channels_hashtable, channel);
    if (lchannel == NULL) {
        lchannel = (struct laser_channel*) calloc(1, sizeof(struct laser_channel));
        unsigned int capacity = bot_gtk_param_widget_get_int(self->pw, PARAM_MEMORY);
        lchannel->data = bot_ptr_circular_new(capacity, free_ldata, NULL);
        lchannel->complained = 0;
        
        printf ("received laser data on channel %s...\n", channel);

        strcpy(lchannel->name, channel);
        g_hash_table_insert(self->channels_hashtable, lchannel->name, lchannel);
        g_ptr_array_add(self->channels, lchannel);

        char key[256];
        sprintf(key, "planar_lidars.%s.viewer_color", channel);

        int sz = bot_conf_get_double_array(globals_get_config(), key, lchannel->color, 3);
        if (sz != 3) {
            printf("%s : missing or funny color!\n", key);
            lchannel->color[0] = 1;
            lchannel->color[1] = 1;
            lchannel->color[2] = 1;
        }
    }

    struct laser_data *ldata = (struct laser_data*) calloc(1, sizeof(struct laser_data));

    if(!atrans_get_trans(self->atrans, channel, "local", &ldata->to_local)) {
        if (!lchannel->complained)
            printf("Didn't get %s -> local transformation\n", channel);
        lchannel->complained = 1;
        return;
    }
    bot_trans_get_mat_3x4(&ldata->to_local, ldata->to_local_mat);

//    if (rlconf_sensor_to_local_matrix_with_pose(self->config, channel, ldata->m, &lpose)) {
//        if (!lchannel->complained)
//            printf("Didn't get calibration for %s\n", channel);
//        lchannel->complained = 1;
//        return;
//    }

    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SPATIAL_DECIMATE) &&
        bot_gtk_param_widget_get_int(self->pw, PARAM_MEMORY) > 10 && bot_ptr_circular_size(lchannel->data) > 0) {
        struct laser_data *last_ldata = bot_ptr_circular_index(lchannel->data, 0);

        double dist = bot_vector_dist_2d(last_ldata->to_local.trans_vec, ldata->to_local.trans_vec);

        if (dist < 0.2) {
            free(ldata);
            return;
        }
    }

    ldata->msg = botlcm_planar_lidar_t_copy(msg);
    bot_ptr_circular_add(lchannel->data, ldata);

    viewer_request_redraw(self->viewer);
    return;
}

static void on_param_widget_changed (BotGtkParamWidget *pw, const char *name, void *user)
{
    RendererLaser *self = (RendererLaser*) user;
    unsigned int capacity = bot_gtk_param_widget_get_int(self->pw, PARAM_MEMORY);

    for (unsigned int i = 0; i < bot_g_ptr_array_size(self->channels); i++) {
        struct laser_channel *lchannel = g_ptr_array_index(self->channels, i);
        bot_ptr_circular_resize(lchannel->data, capacity);
    }

    viewer_request_redraw(self->viewer);
}

static void on_load_preferences (Viewer *viewer, GKeyFile *keyfile, void *user_data)
{
    RendererLaser *self = user_data;
    bot_gtk_param_widget_load_from_key_file (self->pw, keyfile, RENDERER_NAME);
}

static void on_save_preferences (Viewer *viewer, GKeyFile *keyfile, void *user_data)
{
    RendererLaser *self = user_data;
    bot_gtk_param_widget_save_to_key_file (self->pw, keyfile, RENDERER_NAME);
}

void setup_renderer_laser(Viewer *viewer, int priority) 
{
    RendererLaser *self = 
        (RendererLaser*) calloc(1, sizeof(RendererLaser));

    Renderer *renderer = &self->renderer;

    renderer->draw = my_draw;
    renderer->destroy = my_free;
    renderer->name = RENDERER_NAME;
    self->pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());
    renderer->widget = GTK_WIDGET(self->pw);
    renderer->enabled = 1;
    renderer->user = self;

    self->lcm = globals_get_lcm();
    self->atrans = globals_get_atrans();
    self->config = globals_get_config();
    self->channels_hashtable = g_hash_table_new(g_str_hash, g_str_equal);
    self->channels = g_ptr_array_new();
    self->viewer = viewer;

    bot_gtk_param_widget_add_int (self->pw, PARAM_MEMORY, 
                               BOT_GTK_PARAM_WIDGET_SLIDER, 1, 200, 1, 1);

    bot_gtk_param_widget_add_enum( self->pw, COLOR_MENU, BOT_GTK_PARAM_WIDGET_MENU,
                                COLOR_LASER,
                                "Laser", COLOR_LASER,
                                "Drab", COLOR_DRAB,
                                "Intensity", COLOR_INTENSITY,
                                NULL);

    bot_gtk_param_widget_add_booleans (self->pw, 0, PARAM_SPATIAL_DECIMATE, 0, NULL);
    bot_gtk_param_widget_add_booleans (self->pw, 0, PARAM_BIG_POINTS, 0, NULL);

    g_signal_connect (G_OBJECT (self->pw), "changed", 
                      G_CALLBACK (on_param_widget_changed), self);
    
    botlcm_planar_lidar_t_subscribe(self->lcm, "SKIRT_.*", on_laser, self);
    botlcm_planar_lidar_t_subscribe(self->lcm, "BROOM_.*", on_laser, self);
    botlcm_planar_lidar_t_subscribe(self->lcm, "HOKUYO_.*", on_laser, self);

    viewer_add_renderer(viewer, renderer, priority);

    g_signal_connect (G_OBJECT (viewer), "load-preferences", 
            G_CALLBACK (on_load_preferences), self);
    g_signal_connect (G_OBJECT (viewer), "save-preferences",
            G_CALLBACK (on_save_preferences), self);

}
