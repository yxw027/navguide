#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <gtk/gtk.h>

#include <bot/bot_core.h>
#include <bot/viewer/viewer.h>

#include <common/udp_util.h>
#include <common/navconf.h>
#include <common/globals.h>
#include <common/atrans.h>

typedef struct {
    Viewer *viewer;
    GPtrArray *camtranses;
    CamTrans *active_camtrans;
    char **cam_names;
    lcm_t *lcm;
    ATrans *atrans;
//    const char *view_mode;
} app_t;

static int 
logplayer_remote_on_key_press(Viewer *viewer, EventHandler *ehandler, 
        const GdkEventKey *event)
{
    int keyval = event->keyval;
    uint8_t cmd = 0;

    switch (keyval)
    {
    case 'P':
    case 'p':
        udp_send_string ((const char*)"127.0.0.1", (int)53261, (char*)"PLAYPAUSETOGGLE");
        break;
    case 'N':
    case 'n':
        udp_send_string((const char*)"127.0.0.1", (int)53261, (char*)"STEP");
        break;
    case '=':
    case '+':
        udp_send_string((const char*)"127.0.0.1", (int)53261, (char*)"FASTER");
        break;
    case '_':
    case '-':
        udp_send_string((const char*)"127.0.0.1", (int)53261, (char*)"SLOWER");
        break;
    case '[':
        udp_send_string((const char*)"127.0.0.1", (int)53261, (char*)"BACK5");
        break;
    case ']':
        udp_send_string((const char*)"127.0.0.1", (int)53261, (char*)"FORWARD5");
        break;
    default:
        return 0;
    }

    return 1;
}

/////////////////////////////////////////////////////////////

#if 0
void setup_renderer_debug(Viewer *viewer, int priority);
void setup_renderer_goal(Viewer *viewer, int priority);
#endif

void setup_renderer_lcmgl(Viewer *viewer, int priority);
void setup_renderer_image(Viewer *viewer, int priority);
void setup_renderer_guidance (Viewer *viewer, int render_priority);
void setup_renderer_scrolling_plots (Viewer *viewer, int render_priority);
#if 0

void setup_renderer_scrolling_plots(Viewer *viewer, int priority);
void setup_renderer_adu(Viewer *viewer, int priority);
void setup_renderer_motion_plan (Viewer *viewer, int render_priority);
void setup_renderer_rrt_debug (Viewer *viewer, int render_priority);
void setup_renderer_sim_motion_plan (Viewer *viewer, int render_priority);
void setup_renderer_simobs (Viewer *viewer, int render_priority);
void setup_renderer_sensor_placement (Viewer *viewer, int render_priority);
void setup_menu_simstate (Viewer *viewer, int render_priority);
void setup_renderer_tracks(Viewer *viewer, int priority);
void setup_renderer_console(Viewer *viewer, int priority);
void setup_renderer_simtraffic(Viewer *viewer, int priority);
void setup_renderer_statusline(Viewer *viewer, int priority);
//void setup_renderer_procman (Viewer *viewer, int render_priority);
void setup_renderer_simcurblines (Viewer *viewer, int render_priority);

typedef struct {
    GtkLabel *label;
    ATrans *atrans;
} mouse_pos_info_data_t;

static int
mouse_pos_info_on_mouse_motion (Viewer *viewer, EventHandler *ehandler,
        const double ray_start[3], const double ray_dir[3], 
        const GdkEventMotion *event)
{
    mouse_pos_info_data_t *mpid = ehandler->user;

    double carpos[3];
    if (atrans_have_trans(mpid->atrans, "body", "local")) {
        atrans_vehicle_pose_local(mpid->atrans, carpos);
    } else {
        carpos[0] = carpos[1] = carpos[2] = 0;
    }

    point2d_t p;
    int status = geom_ray_z_plane_intersect_3d(POINT3D (ray_start),
            POINT3D (ray_dir), carpos[2], &p);
    if (0 == status) {
        char buf[40];
        snprintf (buf, sizeof (buf), "<%.2f, %.2f>", p.x, p.y);
        gtk_label_set_text (mpid->label, buf);
        gtk_widget_set_sensitive (GTK_WIDGET (mpid->label), TRUE);
    } else {
        gtk_label_set_text (mpid->label, "<??, \?\?>");
        gtk_widget_set_sensitive (GTK_WIDGET (mpid->label), FALSE);
    }
    return 0;
}

static void
add_mouse_position_info_event_handler (Viewer *viewer)
{
    GtkToolItem *toolitem = gtk_tool_item_new ();
    GtkWidget *label = gtk_label_new ("<??, \?\?>");
    gtk_widget_set_sensitive (label, FALSE);
    gtk_container_add (GTK_CONTAINER (toolitem), label);
    GtkToolItem *separator = gtk_separator_tool_item_new ();
    gtk_widget_show (GTK_WIDGET (separator));
    gtk_toolbar_insert (viewer->toolbar, separator, -1);
    gtk_toolbar_insert (viewer->toolbar, toolitem, -1);
    gtk_widget_show_all (GTK_WIDGET (toolitem));

    EventHandler *ehandler = (EventHandler*) calloc(1, sizeof(EventHandler));
    ehandler->name = "Mouse Position Info";
    ehandler->enabled = 1;
    ehandler->mouse_motion = mouse_pos_info_on_mouse_motion;
    viewer_add_event_handler(viewer, ehandler, 0);
    mouse_pos_info_data_t *mpid = g_slice_new0 (mouse_pos_info_data_t);
    mpid->label = GTK_LABEL (label);
    mpid->atrans = globals_get_atrans ();
    ehandler->user = mpid;
}
#endif

static inline double
get_cam_vertical_fov (CamTrans *camtrans)
{
    double cop_x = camtrans_get_principal_x (camtrans);
    double cop_y = camtrans_get_principal_y (camtrans);
    double cam_height = camtrans_get_image_height (camtrans);

    double upper[3], lower[3], middle[3];
    camtrans_unproject_distorted(camtrans, cop_x, 0, upper);
    camtrans_unproject_distorted(camtrans, cop_x, cop_y, middle);
    camtrans_unproject_distorted(camtrans, cop_x, cam_height, lower);

    // since the center of projection may not be in the middle of the image,
    // the angle from the COP to the top pixel may not be the same as the angle
    // from the COP to the bottom pixel.  Set the FOV to be twice the
    // smaller of these two angles, so that the entire vertical FOV is within
    // the image extents
    double theta1 = bot_vector_angle_3d (upper, middle);
    double theta2 = bot_vector_angle_3d (lower, middle);
    double fov = MIN (theta1, theta2) * 2;
    return fov;
}

static void
on_camera_view_handler_rmi_activate (GtkRadioMenuItem *rmi, void *user_data)
{
    if(! gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(rmi)))
        return;
    app_t *app = (app_t*) user_data;
    CamTrans *camtrans = (CamTrans*)g_object_get_data(G_OBJECT(rmi), "camtrans");

    if(app->active_camtrans != camtrans) {
        app->active_camtrans = camtrans;
        double fov_y = get_cam_vertical_fov(app->active_camtrans) * 180 / M_PI;
        ViewHandler *vhandler = app->viewer->view_handler;
        vhandler->set_camera_perspective(vhandler, fov_y);
    }

    viewer_request_redraw(app->viewer);
}

static void on_perspective_item(GtkMenuItem *mi, void *user)
{
    if(! gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mi)))
        return;
    app_t *app = (app_t*) user;
    ViewHandler *vhandler = app->viewer->view_handler;
    if(vhandler) {
        app->active_camtrans = NULL;
        vhandler->set_camera_perspective(vhandler, 60);
    }
}

static void on_orthographic_item(GtkMenuItem *mi, void *user)
{
    if(! gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mi)))
        return;
    app_t *app = (app_t*) user;
    ViewHandler *vhandler = app->viewer->view_handler;
    if(vhandler) {
        app->active_camtrans = NULL;
        vhandler->set_camera_orthographic(vhandler);
    }
}

static void
on_render_begin(Viewer *viewer, void *user_data)
{
    app_t *app = (app_t*)user_data;
    if(!app->active_camtrans)
        return;
    ViewHandler *vhandler = app->viewer->view_handler;
    double look_at_cam[3] = { 0, 0, 1 }, look_at_local[3];
    double up_cam[3] = { 0, -1, 0 }, up_local[3];

    const char *cam_name = camtrans_get_name(app->active_camtrans);
    BotTrans cam_to_local;
    if(!atrans_get_trans(app->atrans, cam_name, "local", &cam_to_local))
        return;
    bot_trans_apply_vec(&cam_to_local, look_at_cam, look_at_local);
    bot_trans_rotate_vec(&cam_to_local, up_cam, up_local);
    const double *cam_pos_local = cam_to_local.trans_vec;
    vhandler->set_look_at(vhandler, cam_pos_local, look_at_local, up_local);
}

static void
add_view_handlers (app_t *app)
{
    Viewer *viewer = app->viewer;
    GtkWidget *view_menuitem = gtk_menu_item_new_with_mnemonic("_View");
    gtk_menu_bar_append(GTK_MENU_BAR(viewer->menu_bar), view_menuitem);

    GtkWidget *view_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_menuitem), view_menu);

    GSList *view_list = NULL;
    GtkWidget *perspective_item = gtk_radio_menu_item_new_with_label(view_list, 
            "Perspective");
    view_list = 
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(perspective_item));
    gtk_menu_append(GTK_MENU(view_menu), perspective_item);
    g_signal_connect(G_OBJECT(perspective_item), "activate", 
            G_CALLBACK(on_perspective_item), app);

    GtkWidget *orthographic_item = 
        gtk_radio_menu_item_new_with_label(view_list, "Orthographic");
    view_list = 
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(orthographic_item));
    gtk_menu_append(GTK_MENU(view_menu), orthographic_item);
    g_signal_connect(G_OBJECT(orthographic_item), "activate", 
            G_CALLBACK(on_orthographic_item), app);

    // add camera view handlers
    BotConf *config = globals_get_config();
    app->cam_names = navconf_get_all_camera_names(config); 
    for (int i=0; app->cam_names && app->cam_names[i]; i++) {
        char *cam_name = app->cam_names[i];
        view_list = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM(perspective_item));

        CamTrans* camtrans = navconf_get_new_camtrans (config, cam_name);
        if (!camtrans)
            return;

        char *label = g_strdup_printf ("Camera POV: %s", cam_name);
        GtkWidget *cvh_rmi = gtk_radio_menu_item_new_with_label (view_list, label);
        free (label);
        gtk_menu_shell_append (GTK_MENU_SHELL (view_menu), cvh_rmi);
        gtk_widget_show (cvh_rmi);

//        g_object_set_data(G_OBJECT(cvh_rmi), "cam_name", cam_name);
        g_object_set_data (G_OBJECT (cvh_rmi), "camtrans", camtrans);
        g_signal_connect (G_OBJECT (cvh_rmi), "activate", 
                G_CALLBACK (on_camera_view_handler_rmi_activate), app);
    }
    globals_release_config (config);

    gtk_widget_show_all(view_menuitem);
}

int main(int argc, char *argv[])
{
    gtk_init (&argc, &argv);
    glutInit (&argc, argv);
    g_thread_init (NULL);

    setlinebuf (stdout);
    
    app_t app;
    memset(&app, 0, sizeof(app));
    app.active_camtrans = NULL;
    app.camtranses = g_ptr_array_new();

    Viewer *viewer = viewer_new("Navguide Viewer");
//    viewer->backgroundColor[0] = 0.8;
//    viewer->backgroundColor[1] = 0.8;
//    viewer->backgroundColor[2] = 0.8;
//    viewer->backgroundColor[3] = 1;
    app.viewer = viewer;
    app.lcm = globals_get_lcm();
    assert (app.lcm);
    app.atrans = globals_get_atrans();
//    botlcm_pose_t_subscribe(app.lcm, "POSE", on_pose, &app);

#if 0

#endif
   setup_renderer_image(viewer, 1);
   setup_renderer_guidance(viewer, 1);
   setup_renderer_scrolling_plots(viewer, 1);
   setup_renderer_lcmgl(viewer, 1);

    // logplayer controls
    EventHandler *ehandler = (EventHandler*) calloc(1, sizeof(EventHandler));
    ehandler->name = (char*)"LogPlayer Remote";
    ehandler->enabled = 1;
    ehandler->key_press = logplayer_remote_on_key_press;
    viewer_add_event_handler(viewer, ehandler, 0);
    add_view_handlers (&app);

#if 0
    add_mouse_position_info_event_handler (viewer);
#endif

    g_signal_connect(G_OBJECT(viewer), "render-begin",
            G_CALLBACK(on_render_begin), &app);

    char *fname = g_build_filename (g_get_user_config_dir(), ".lanes-viewerrc", 
            NULL);
    viewer_load_preferences (viewer, fname);
    gtk_main ();  
    viewer_save_preferences (viewer, fname);
    free (fname);
    viewer_unref (viewer);

    globals_release_lcm(app.lcm);
    globals_release_atrans(app.atrans);
    g_strfreev(app.cam_names);
    for (int i=0; i<app.camtranses->len; i++) {
        CamTrans *camtrans = (CamTrans*) g_ptr_array_index(app.camtranses, i);
        camtrans_destroy(camtrans);
    }
    g_ptr_array_free(app.camtranses, TRUE);
}
