#include "events.h"
#include "main.h"

extern "C" void on_frame_clear (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    // which frame to apply to
    gint id = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "which_frame")));

    if (id == 0) {
        clear_data (self, TRUE, FALSE);
    }
    if (id == 1) {
        clear_data (self, FALSE, TRUE);
    }
}

extern "C" void on_frame_apply (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;

    // check data sanity
    for (int i=0;i<NSENSORS;i++) {
        if (!self->img[i]) {
            fprintf (stderr, "did not receive image for sensor %d.\n", i);
            return;
        }
    }

    // which frame to apply to
    gint id = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "which_frame")));

    if (id == 0) {
        clear_data (self, TRUE, FALSE);
        printf ("Applying images to frame 1...\n");
        // copy data
        for (int i=0;i<NSENSORS;i++) {
            self->frame1[i] = botlcm_image_t_copy (self->img[i]);
        }
        if (self->pose) {
            if (self->pose1)
                botlcm_pose_t_destroy (self->pose1);
            self->pose1 = botlcm_pose_t_copy (self->pose);
        }
    }
    if (id == 1) {
        clear_data (self, FALSE, TRUE);
        printf ("Applying images to frame 2...\n");
        // copy data
        for (int i=0;i<NSENSORS;i++) {
            self->frame2[i] = botlcm_image_t_copy (self->img[i]);
        }
        if (self->pose) {
            if (self->pose2)
                botlcm_pose_t_destroy (self->pose2);
            self->pose2 = botlcm_pose_t_copy (self->pose);
        }
    }

}

extern "C" void on_refresh_display_combo_box (GtkComboBox *box, gpointer data)
{
    state_t *self = (state_t*)data;

    gtku_gl_drawing_area_invalidate (self->gl_area_1);
}

extern "C" void on_sift_nscales_changed (GtkSpinButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->sift_nscales = gtk_spin_button_get_value_as_int (button);
    printf ("SIFT nscales set to %d\n", self->features_param->sift_nscales);
}

extern "C" void on_sift_sigma_changed (GtkSpinButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->sift_sigma = gtk_spin_button_get_value_as_float (button);
    printf ("SIFT sigma set to %.3f\n", self->features_param->sift_sigma);
}

extern "C" void on_sift_peakthresh_changed (GtkSpinButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->sift_peakthresh = gtk_spin_button_get_value_as_float (button);
    printf ("SIFT peakthresh set to %.3f\n", self->features_param->sift_peakthresh);
}

extern "C" void on_sift_half_size_changed (GtkToggleButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->scale_factor = gtk_toggle_button_get_active (button) ? .5 : 1.0;

    printf ("SIFT resize set to %.3f\n", self->features_param->scale_factor);
}

extern "C" void on_surf_octaves_changed (GtkSpinButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->surf_octaves = gtk_spin_button_get_value_as_int (button);
    printf ("SURF octaves set to %d\n", self->features_param->surf_octaves);
}

extern "C" void on_surf_intervals_changed (GtkSpinButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->surf_intervals = gtk_spin_button_get_value_as_int (button);
    printf ("SURF intervals set to %d\n", self->features_param->surf_intervals);
}

extern "C" void on_surf_init_sample_changed (GtkSpinButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->surf_init_sample = gtk_spin_button_get_value_as_int (button);
    printf ("SURF init_sample set to %d\n", self->features_param->surf_init_sample);
}

extern "C" void on_surf_thresh_changed (GtkSpinButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->surf_thresh = gtk_spin_button_get_value_as_float (button);
    printf ("SURF thresh set to %.4f\n", self->features_param->surf_thresh);
}

extern "C" void on_fast_thresh_changed (GtkSpinButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    self->features_param->fast_thresh = gtk_range_get_value (GTK_RANGE (glade_xml_get_widget(self->gladexml, "fast_thresh")));
    printf ("FAST thresh set to %d\n", self->features_param->fast_thresh);
}


extern "C" void on_sift_param_reset (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    set_param_defaults (self, SIFT);
}
extern "C" void on_surf_param_reset (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    set_param_defaults (self, SURF);
}

extern "C" void on_compute_features (GtkMenuItem *item, gpointer data)
{
    state_t *self = (state_t*)data;
    
    const char *type = gtk_combo_box_get_active_text (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "feature_type")));

    if (strcmp (type, "SIFT") == 0) 
        self->features_param->feature_type = NAVLCM_FEATURES_PARAM_T_SIFT;
    if (strcmp (type, "SIFT2") == 0) 
        self->features_param->feature_type = NAVLCM_FEATURES_PARAM_T_SIFT2;
    if (strcmp (type, "FAST") == 0) 
        self->features_param->feature_type = NAVLCM_FEATURES_PARAM_T_FAST;
    if (strcmp (type, "SURF64") == 0) 
        self->features_param->feature_type = NAVLCM_FEATURES_PARAM_T_SURF64;
    if (strcmp (type, "SURF128") == 0) 
        self->features_param->feature_type = NAVLCM_FEATURES_PARAM_T_SURF128;
    if (strcmp (type, "GFTT") == 0) 
        self->features_param->feature_type = NAVLCM_FEATURES_PARAM_T_GFTT;

    printf ("feature type: %d\n", self->features_param->feature_type);

    // clear features
    clear_features (self, TRUE, TRUE);

    // compute features for both frames
    self->features1 = features_driver (self->frame1, NSENSORS, self->features_param);
    self->features2 = features_driver (self->frame2, NSENSORS, self->features_param);
  
    printf ("Check: %d %d\n", self->features1->feature_type, self->features2->feature_type);

    //duplicate features
    duplicate_features (self);

    gtku_gl_drawing_area_invalidate (self->gl_area_1);

}

extern "C" void on_match_features (GtkMenuItem *item, gpointer data)
{
    state_t *self = (state_t*)data;

    double thresh = gtk_range_get_value (GTK_RANGE (glade_xml_get_widget(self->gladexml, "match_thresh")));
    gboolean monogamy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
            (glade_xml_get_widget (self->gladexml, "monogamy")));
    gboolean mutual_consistency_check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
            (glade_xml_get_widget (self->gladexml, "mutual_consistency_check")));

    // clear matches
    clear_matches (self);

    // compute matches
    if (self->features1 && self->features2) {
        self->matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
        int matching_mode = self->features1->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
        find_feature_matches_fast (self->features1, self->features2, TRUE, TRUE, monogamy, 
                mutual_consistency_check, thresh, -1, matching_mode, self->matches);
    }

    // compute mismatches
    util_compute_mismatches (self);

    gtku_gl_drawing_area_invalidate (self->gl_area_1);

}

extern "C" void on_start_ground_truth (GtkMenuItem *item, gpointer data)
{
    state_t *self = (state_t*)data;

    if (!self->matches)
        return;

    printf ("erasing ground-truth...\n");

    // erase ground truth
    if (self->gmatches) {
        navlcm_feature_match_set_t_destroy (self->gmatches);
        self->gmatches = NULL;
    }

    if (!self->features1 || !self->features2)
        return;

    // reset features copy
    duplicate_features (self);

    // populate candidates matches from auto matches
    self->gmatches = navlcm_feature_match_set_t_create ();
    for (int i=0;i<self->matches->num;i++) {
        add_match (self, self->matches->el + i);
    }

    // set combobox to "none"
    gtk_combo_box_set_active  (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "match_type")), 2);
}

extern "C" void on_span_matching_threshold (GtkMenuItem *menu, gpointer data)
{
    state_t *self = (state_t*)data;

    util_span_matching_threshold (self);
}

extern "C" void on_match_thresh_value_changed (GtkRange *range, gpointer data)
{
    // refresh matches
    on_match_features (NULL, data);

}

extern "C" void on_feature_type_changed (GtkComboBox *box, gpointer data)
{
    // refresh features
    on_compute_features (NULL, data);
}

extern "C" void on_match_type_changed (GtkComboBox *box, gpointer data)
{
    state_t *self = (state_t*)data;

    gtku_gl_drawing_area_invalidate (self->gl_area_1);
}

extern "C" void on_load_data (GtkMenuItem *item, gpointer data)
{
    state_t *self = (state_t*)data;

    // prompt user for file
    GtkWidget *dialog;

    char *filename = NULL;

    dialog = gtk_file_chooser_dialog_new ("Select input file",
            NULL,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
            NULL);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        read_data_from_file (self, filename);

        g_free (filename);

    }

    gtk_widget_destroy (dialog);

}

extern "C" void on_save_data (GtkMenuItem *item, gpointer data)
{
    state_t *self = (state_t*)data;
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new ("Select output file",
            NULL,
            GTK_FILE_CHOOSER_ACTION_SAVE,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
            NULL);

    gtk_file_chooser_set_do_overwrite_confirmation 
        (GTK_FILE_CHOOSER (dialog), TRUE);

    char *filename = NULL;

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        save_data_to_file (self, filename);

        g_free (filename);
    }

    gtk_widget_destroy (dialog);
}

