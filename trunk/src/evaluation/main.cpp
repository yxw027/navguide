#include "main.h"


static gboolean on_gl_expose_1 (GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
    state_t * self = (state_t *) user_data;

    int width = widget->allocation.width;
    int height = widget->allocation.height;

    int match_type = gtk_combo_box_get_active  (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "match_type")));

    navlcm_feature_match_set_t *matches = NULL;
    navlcm_feature_list_t *features1 = self->features1;
    navlcm_feature_list_t *features2 = self->features2;
    navlcm_feature_match_t *match = NULL;

    if (match_type == 0) { // auto matches
        matches = self->matches;
    }
    if (match_type == 1) { // ground-truth matches
        matches = self->gmatches;
        features1 = self->features1c;
        features2 = self->features2c;
    }
    if (match_type == 2) { // no matches
        features1 = self->features1c;
        features2 = self->features2c;
    }
    if (match_type == 3) { // mismatches only
        matches = self->mismatches;
    }

    // this is were we draw the stuff

    if (gtku_gl_drawing_area_set_context (self->gl_area_1)!=0) {
        return TRUE;
    }

    assert (check_gl_error ("0"));

    set_background_color (self);

    glDisable (GL_DEPTH_TEST);

    setup_2d (width, height, 0, width, height, 0);

    // compute precision and recall
    double precision = compute_precision (self->matches, self->gmatches);
    double recall = compute_recall (self->matches, self->gmatches);

    // draw stuff
    draw_frame (width, height, self->frame1[0] ? self->frame1 : self->img, self->frame2[0] ? self->frame2 : self->img, NSENSORS, features1, features2, matches, self->user_scale_1, self->user_trans_1, self->feature_finder_pressed, FALSE, self->highlight_f, self->highlight_m, self->match_f1, self->match_f2, self->select_m, match, self->features_param->feature_type, precision, recall);

    assert (check_gl_error ("1"));

    // display pose info
    if (self->pose1 && (self->pose || self->pose2)) {
        botlcm_pose_t *pose = self->pose2 ? self->pose2 : self->pose;
        double d = sqrt (powf (self->pose1->pos[0]-pose->pos[0],2) + 
                powf (self->pose1->pos[1]-pose->pos[1],2) + 
                powf (self->pose1->pos[2]-pose->pos[2],2));

        display_msg (10, height-40, width, height, RED, 0, "pose dist: %.2f m.", d);
    }

    // start picking
    start_picking (self->mouse_x, self->mouse_y, width, height);

    // draw stuff in select mode
    draw_frame (width, height, self->frame1[0] ? self->frame1 : self->img, self->frame2[0] ? self->frame2 : self->img, NSENSORS, features1, features2, matches, self->user_scale_1, self->user_trans_1, FALSE, TRUE, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0);

    assert (check_gl_error ("0"));
    // stop picking
    GLuint type=0, name=0;
    if (stop_picking (&type, &name) == 0) {
        if (type == NAME_POINT1) self->highlight_f = features1->el + name;
        if (type == NAME_POINT2) self->highlight_f = features2->el + name;
        if (type == NAME_MATCH) { self->highlight_m = matches->el + name; }
    } else {
        self->highlight_f = NULL;
        self->highlight_m = NULL;
    }
    gtku_gl_drawing_area_swap_buffers (self->gl_area_1);

    return TRUE;
}

gboolean on_gl_scroll_1 (GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
    double s = 1.10;

    state_t *self = (state_t*)data;
    if (event->direction == GDK_SCROLL_UP) {
        self->user_scale_1 *= s;
        self->user_trans_1[0] += (1.0-s) * (event->x - self->user_trans_1[0]);
        self->user_trans_1[1] += (1.0-s) * (event->y - self->user_trans_1[1]);
    } 
    else {
        self->user_scale_1 /= s;
        self->user_trans_1[0] += -(1.0/s-1) * (event->x - self->user_trans_1[0]);
        self->user_trans_1[1] += -(1.0/s-1) * (event->y - self->user_trans_1[1]);
    }

    gtku_gl_drawing_area_invalidate (self->gl_area_1);

    return TRUE;
}
gboolean on_gl_enter_notify_1 ( GtkWidget *widget, GdkEventMotion *event, void *user_data )
{

    GTK_WIDGET_SET_FLAGS( widget, GTK_CAN_FOCUS );

    if(GTK_WIDGET_REALIZED(widget)) {
        gtk_widget_grab_focus( widget );
    }

    return TRUE;
}

gboolean on_gl_key_press_1 (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    state_t *self = (state_t*)data;

    util_on_key_press (self, event->keyval);

    gtku_gl_drawing_area_invalidate (self->gl_area_1);

    return TRUE;
}

gboolean on_gl_key_release_1 (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    state_t *self = (state_t*)data;

    util_on_key_release (self, event->keyval);

    gtku_gl_drawing_area_invalidate (self->gl_area_1);

    return TRUE;
}

gboolean on_gl_button_press_1 (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    state_t *self = (state_t*)data;

    util_on_button_press (self, event->button);

    gtku_gl_drawing_area_invalidate (self->gl_area_1);

    return TRUE;
}

gboolean on_gl_motion_1 (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    state_t *self = (state_t*)data;

    self->mouse_x = event->x;
    self->mouse_y = event->y;

    gtku_gl_drawing_area_invalidate (self->gl_area_1);

    return TRUE;
}

static void on_botlcm_image_event (const lcm_recv_buf_t *buf, const char *channel, 
        const botlcm_image_t *msg, void *user)
{
    state_t *self = (state_t*)user;

    int sensor_id = -1;

    // determine channel ID
    for (int i=0;i<NSENSORS;i++) {
        if (strcmp (channel, CHANNEL_NAMES[i]) == 0) {
            sensor_id = i;
        }
    }

    if (sensor_id == -1) return;

    // skip if not supposed to listen
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget(self->gladexml, "listen_to_lcm_images"))))
        return;

    if (self->img[sensor_id]) 
        botlcm_image_t_destroy (self->img[sensor_id]);
    self->img[sensor_id] = botlcm_image_t_copy (msg);

    gtku_gl_drawing_area_invalidate (self->gl_area_1);
}

void set_param_defaults (state_t *self, int type)
{
    if (type == SIFT) {
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_nscales")), SIFT_NSCALES);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_sigma")), SIFT_SIGMA);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_peakthresh")), SIFT_PEAKTHRESH);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget(self->gladexml, "sift_half_size")), 0);    
        self->features_param->sift_doubleimsize = SIFT_DOUBLEIMSIZE;
        self->features_param->scale_factor= 1.0;
    }
    if (type == SURF) {
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_octaves")), SURF_OCTAVES);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_intervals")), SURF_INTERVALS);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_init_sample")), SURF_INIT_SAMPLE);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_thresh")), SURF_THRESH);
    }

}

static void setup_gtk( state_t *self )
{
    self->gladexml = glade_xml_new(GLADE_FILE, NULL, NULL);

    // automagically connect signal handlers
    glade_xml_signal_autoconnect( self->gladexml );

    // setup the OpenGL output area for the "Frame1" window
    GtkWidget * frame1 = glade_xml_get_widget(self->gladexml, "frame1");
    self->gl_area_1 = GTKU_GL_DRAWING_AREA (gtku_gl_drawing_area_new (FALSE));

    gtk_widget_set_events(GTK_WIDGET(self->gl_area_1), 
            GDK_LEAVE_NOTIFY_MASK |
            GDK_ENTER_NOTIFY_MASK |
            GDK_BUTTON_PRESS_MASK | 
            GDK_BUTTON_RELEASE_MASK | 
            GDK_POINTER_MOTION_MASK | 
            GDK_KEY_PRESS_MASK |
            GDK_KEY_RELEASE_MASK);

    gtk_container_add (GTK_CONTAINER (frame1), GTK_WIDGET (self->gl_area_1));

    // glarea signals
    g_signal_connect (G_OBJECT (self->gl_area_1), "expose-event", G_CALLBACK(on_gl_expose_1), self);
    g_signal_connect (G_OBJECT (self->gl_area_1), "scroll-event", G_CALLBACK(on_gl_scroll_1), self);
    g_signal_connect (G_OBJECT (self->gl_area_1), "button-press-event", G_CALLBACK(on_gl_button_press_1), self);
    g_signal_connect (G_OBJECT (self->gl_area_1), "motion-notify-event", G_CALLBACK(on_gl_motion_1), self);
    g_signal_connect (G_OBJECT (self->gl_area_1), "key-press-event", G_CALLBACK(on_gl_key_press_1), self);
    g_signal_connect (G_OBJECT (self->gl_area_1), "key-release-event", G_CALLBACK(on_gl_key_release_1), self);
    g_signal_connect (G_OBJECT (self->gl_area_1), "enter-notify-event", G_CALLBACK(on_gl_enter_notify_1), self);

    // other signals
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "frame_apply")), "clicked",
            G_CALLBACK (on_frame_apply), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "frame_clear")), "clicked",
            G_CALLBACK (on_frame_clear), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "background_color")), "changed",
            G_CALLBACK (on_refresh_display_combo_box), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "sift_nscales")), "value-changed",
            G_CALLBACK (on_sift_nscales_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "sift_sigma")), "value-changed",
            G_CALLBACK (on_sift_sigma_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "sift_peakthresh")), "value-changed",
            G_CALLBACK (on_sift_peakthresh_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "sift_half_size")), "toggled",
            G_CALLBACK (on_sift_half_size_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "surf_octaves")), "value-changed",
            G_CALLBACK (on_surf_octaves_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "surf_intervals")), "value-changed",
            G_CALLBACK (on_surf_intervals_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "surf_init_sample")), "value-changed",
            G_CALLBACK (on_surf_init_sample_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "surf_thresh")), "value-changed",
            G_CALLBACK (on_surf_thresh_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "fast_thresh")), "value-changed",
            G_CALLBACK (on_fast_thresh_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "sift_param_reset")), "clicked",
            G_CALLBACK (on_sift_param_reset), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "surf_param_reset")), "clicked",
            G_CALLBACK (on_surf_param_reset), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "compute_features")), "activate",
            G_CALLBACK (on_compute_features), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "start_ground_truth")), "activate",
            G_CALLBACK (on_start_ground_truth), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "match_features")), "activate",
            G_CALLBACK (on_match_features), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "span_matching_threshold")), "activate",
            G_CALLBACK (on_span_matching_threshold), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "match_thresh")), "value-changed",
            G_CALLBACK (on_match_thresh_value_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "feature_type")), "changed",
            G_CALLBACK (on_feature_type_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "match_type")), "changed",
            G_CALLBACK (on_match_type_changed), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "load_data")), "activate",
            G_CALLBACK (on_load_data), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget(self->gladexml, "save_data")), "activate", 
            G_CALLBACK (on_save_data), self);

    // set parameters increments
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_nscales")), 1, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_sigma")), .001, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_peakthresh")), .001, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_octaves")), 1, 0);

    // set parameters increments
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_nscales")), 1, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_sigma")), .001, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_peakthresh")), .001, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_octaves")), 1, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_intervals")), 1, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_init_sample")), 1, 0);
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_thresh")), .0001, 0);
    gtk_range_set_increments       (GTK_RANGE       (glade_xml_get_widget(self->gladexml, "match_thresh")), .01, 0);
    gtk_range_set_increments       (GTK_RANGE       (glade_xml_get_widget(self->gladexml, "fast_thresh")), 1, 0);

    // set parameter range
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_nscales")), 1, 10);
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_sigma")), 0, 2);
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_peakthresh")), 0, 3);

    gtk_spin_button_set_range (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_octaves")), 1, 10);
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_intervals")), 1, 10);
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_init_sample")), 1, 10);
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_thresh")), 0, 4000);
    gtk_range_set_range       (GTK_RANGE       (glade_xml_get_widget(self->gladexml, "match_thresh")), 0.0, 1.0);
    gtk_range_set_range       (GTK_RANGE       (glade_xml_get_widget(self->gladexml, "fast_thresh")), 0.0, 100.0);

    // set digits
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_nscales")), 0);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_sigma")), 3);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "sift_peakthresh")), 3);

    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_octaves")), 0);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_intervals")), 0);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_init_sample")), 0);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (glade_xml_get_widget(self->gladexml, "surf_thresh")), 4);
    gtk_scale_set_digits       (GTK_SCALE (glade_xml_get_widget(self->gladexml, "match_thresh")), 2);
    gtk_scale_set_digits       (GTK_SCALE (glade_xml_get_widget(self->gladexml, "fast_thresh")), 0);

    // set default parameter values
    set_param_defaults (self, SIFT);
    set_param_defaults (self, SURF);
    gtk_combo_box_set_active  (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "background_color")), 0);
    gtk_combo_box_set_active  (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "feature_type")), 0);
    gtk_combo_box_set_active  (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "which_frame")), 0);
    gtk_combo_box_set_active  (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "match_type")), 0);
    gtk_range_set_value       (GTK_RANGE     (glade_xml_get_widget(self->gladexml, "match_thresh")), MATCH_THRESH);
    gtk_range_set_value       (GTK_RANGE     (glade_xml_get_widget(self->gladexml, "fast_thresh")), FAST_THRESH);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget(self->gladexml, "listen_to_lcm_images")), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget(self->gladexml, "monogamy")), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget(self->gladexml, "mutual_consistency_check")), 1);

    /*
       g_signal_connect(G_OBJECT(self->gl_area_1), "enter-notify-event",
       G_CALLBACK(on_enter_notify), self);
       g_signal_connect(G_OBJECT(self->gl_area_1), "leave-notify-event",
       G_CALLBACK(on_leave_notify), self);
       g_signal_connect (G_OBJECT(self->gl_area_1), "motion-notify-event",
       G_CALLBACK(on_motion_notify), self);
       g_signal_connect (G_OBJECT(self->gl_area_1), "button-press-event",
       G_CALLBACK(on_button_press), self);
       */

    gtk_widget_show_all (glade_xml_get_widget(self->gladexml, "main_window"));
}

static void on_pose (const lcm_recv_buf_t *buf, const char *channel, const botlcm_pose_t *msg, void *user)
{
    state_t *self = (state_t*)user;

    if (self->pose)
        botlcm_pose_t_destroy (self->pose);

    self->pose = botlcm_pose_t_copy (msg);
}

int main (int argc, char *argv[])
{
    g_type_init ();

    dbg_init ();

    glade_init ();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);

    // initialize application state and zero out memory
    g_self = (state_t*) calloc( 1, sizeof(state_t) );

    state_t *self = g_self;

    // parse arguments
    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_string  (gopt, 'f',   "file",    "",     "Input data file");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    // setup variables
    self->lcm = lcm_create(NULL);
    if (!self->lcm)
        return 1;

    self->features_param = (navlcm_features_param_t*)malloc(sizeof(navlcm_features_param_t));

    self->features1 = NULL;
    self->features2 = NULL;
    self->features1c = NULL;
    self->features2c = NULL;
    self->matches = NULL;
    self->gmatches = NULL;
    self->mismatches = NULL;
    self->user_scale_1 = 1.0;
    self->user_trans_1[0] = .0;
    self->user_trans_1[1] = .0;
    self->highlight_f = NULL;
    self->highlight_m = NULL;
    self->match_f1 = NULL;
    self->select_m = NULL;
    self->history = g_queue_new ();
    self->feature_finder_pressed = FALSE;
    self->pose = NULL;
    self->pose1 = NULL;
    self->pose2 = NULL;

    for (int i=0;i<NSENSORS;i++) {
        self->img[i] = NULL;
        self->frame1[i] = NULL;
        self->frame2[i] = NULL;
    }

    // Initialize GTK
    gtk_init(&argc, &argv);

    setup_gtk (self);

    // read data
    if (file_exists (getopt_get_string (gopt, "file"))) {
        printf ("reading file %s\n", getopt_get_string (gopt, "file"));
        read_data_from_file (self, getopt_get_string (gopt, "file"));
    } else {
        fprintf (stderr, "failed to read file %s\n", getopt_get_string (gopt, "file"));
    }

    // subscribe to images
    for (int i=0;i<NSENSORS;i++) {
        botlcm_image_t_subscribe (self->lcm, CHANNEL_NAMES[i], on_botlcm_image_event, self);
    }

    botlcm_pose_t_subscribe (self->lcm, "POSE", on_pose, self);

    // attach lcm to main loop
    glib_mainloop_attach_lcm (self->lcm);

    // run the GTK main loop
    gtku_quit_on_interrupt ();
    gtk_main ();

    return 0;
}

