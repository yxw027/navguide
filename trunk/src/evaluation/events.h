#ifndef _EVENTS_H__
#define _EVENTS_H__

#include <stdlib.h>
#include <stdio.h>
#include <features/util.h>

#include <gtk/gtk.h>

#include <guidance/matcher.h>

extern "C" void on_frame_apply (GtkButton *button, gpointer data);
extern "C" void on_frame_clear (GtkButton *button, gpointer data);
extern "C" void on_refresh_display_combo_box (GtkComboBox *box, gpointer data);
extern "C" void on_sift_nscales_changed (GtkSpinButton *button, gpointer data);
extern "C" void on_sift_sigma_changed (GtkSpinButton *button, gpointer data);
extern "C" void on_sift_peakthresh_changed (GtkSpinButton *button, gpointer data);
extern "C" void on_sift_half_size_changed (GtkToggleButton *button, gpointer data);
extern "C" void on_surf_octaves_changed (GtkSpinButton *button, gpointer data);
extern "C" void on_surf_intervals_changed (GtkSpinButton *button, gpointer data);
extern "C" void on_surf_init_sample_changed (GtkSpinButton *button, gpointer data);
extern "C" void on_fast_thresh_changed (GtkSpinButton *button, gpointer data);
extern "C" void on_surf_thresh_changed (GtkSpinButton *button, gpointer data);
extern "C" void on_sift_param_reset (GtkButton *button, gpointer data);
extern "C" void on_surf_param_reset (GtkButton *button, gpointer data);
extern "C" void on_compute_features (GtkMenuItem *item, gpointer data);
extern "C" void on_match_features (GtkMenuItem *item, gpointer data);
extern "C" void on_span_matching_threshold (GtkMenuItem *menu, gpointer data);
extern "C" void on_start_ground_truth (GtkMenuItem *item, gpointer data);
extern "C" void on_match_type_changed (GtkComboBox *box, gpointer data);
extern "C" void on_feature_type_changed (GtkComboBox *box, gpointer data);
extern "C" void on_match_thresh_value_changed (GtkRange *range, gpointer data);
extern "C" void on_load_data (GtkMenuItem *item, gpointer data);
extern "C" void on_save_data (GtkMenuItem *item, gpointer data);

#endif

