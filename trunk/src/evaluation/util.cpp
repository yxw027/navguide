#include "main.h"
#include <gdk/gdkkeysyms.h>

#define MGK_FRAME1 0
#define MGK_FRAME2 1
#define MGK_FEATURES1 2
#define MGK_FEATURES2 3
#define MGK_MATCHES 4
#define MGK_GMATCHES 5
#define MGK_FEATURES1C 6
#define MGK_FEATURES2C 7
#define MGK_MISMATCHES 8

#define HISTORY_DELETE_KEYPOINT 0
#define HISTORY_ADD_MATCH 1
#define HISTORY_DELETE_MATCH 2

void save_data_to_file (state_t *self, const char *filename)
{
    char code=0;
    int fw = 0;

    FILE *fp = fopen (filename, "wb");
    if (!fp) {
        fprintf (stderr, "failed to open file %s in wb mode.\n", filename);
        return;
    }
    
    printf ("writing data to file %s\n", filename);

    // write images
    for (int i=0;i<NSENSORS;i++) {
        code = MGK_FRAME1;
        if (self->frame1[i]) {
            fw = fwrite (&code, sizeof(char), 1, fp);
            botlcm_image_t_write (self->frame1[i], fp);
        }
    }
    for (int i=0;i<NSENSORS;i++) {
        code = MGK_FRAME2;
        if (self->frame2[i]) {
            fw = fwrite (&code, sizeof(char), 1, fp);
            botlcm_image_t_write (self->frame2[i], fp);
        }
    }

    // write features
    if (self->features1) {
        code = MGK_FEATURES1;
        fw = fwrite (&code, sizeof(char), 1, fp);
        navlcm_feature_list_t_write (self->features1, fp);
    }
    if (self->features2) {
        code = MGK_FEATURES2;
        fw = fwrite (&code, sizeof(char), 1, fp);
        navlcm_feature_list_t_write (self->features2, fp);
    }
    if (self->features1c) {
        code = MGK_FEATURES1C;
        fw = fwrite (&code, sizeof(char), 1, fp);
        navlcm_feature_list_t_write (self->features1c, fp);
    }
    if (self->features2c) {
        code = MGK_FEATURES2C;
        fw = fwrite (&code, sizeof(char), 1, fp);
        navlcm_feature_list_t_write (self->features2c, fp);
    }

    // write matches
    if (self->matches) {
        code = MGK_MATCHES;
        fw = fwrite (&code, sizeof(char), 1, fp);
        navlcm_feature_match_set_t_write (self->matches, fp);
    }

    // write matches
    if (self->gmatches) {
        code = MGK_GMATCHES;
        fw = fwrite (&code, sizeof(char), 1, fp);
        navlcm_feature_match_set_t_write (self->gmatches, fp);
    }

    // write matches
    if (self->mismatches) {
        code = MGK_MISMATCHES;
        fw = fwrite (&code, sizeof(char), 1, fp);
        navlcm_feature_match_set_t_write (self->mismatches, fp);
    }

    fclose (fp);
}

void read_data_from_file (state_t *self, const char *filename)
{
    char code=0;
    int fw = 0;
    int count1=0, count2=0;
    botlcm_image_t *img = NULL;
    navlcm_feature_list_t *f = NULL;
    navlcm_feature_match_set_t *m = NULL;

    // clear current data first
    clear_data (self, TRUE, TRUE);

    FILE *fp = fopen (filename, "rb");

    if (!fp) {
        fprintf (stderr, "failed to open file %s in rb mode.\n", filename);
        return;
    }

    printf ("reading data from file %s\n", filename);

    while (fread (&code, sizeof(char), 1, fp) == 1) {
        switch (code) {
            case MGK_FRAME1:
                img = (botlcm_image_t*)malloc(sizeof(botlcm_image_t));
                botlcm_image_t_read (img, fp);
                self->frame1[count1] = img;
                count1++;
                break;
            case MGK_FRAME2:
                img = (botlcm_image_t*)malloc(sizeof(botlcm_image_t));
                botlcm_image_t_read (img, fp);
                self->frame2[count2] = img;
                count2++;
                break;
            case MGK_FEATURES1:
                f = (navlcm_feature_list_t*)malloc(sizeof(navlcm_feature_list_t));
                navlcm_feature_list_t_read (f, fp);
                self->features1 = f;
                break;
            case MGK_FEATURES2:
                f = (navlcm_feature_list_t*)malloc(sizeof(navlcm_feature_list_t));
                navlcm_feature_list_t_read (f, fp);
                self->features2 = f;
                break;
            case MGK_FEATURES1C:
                f = (navlcm_feature_list_t*)malloc(sizeof(navlcm_feature_list_t));
                navlcm_feature_list_t_read (f, fp);
                self->features1c = f;
                break;
            case MGK_FEATURES2C:
                f = (navlcm_feature_list_t*)malloc(sizeof(navlcm_feature_list_t));
                navlcm_feature_list_t_read (f, fp);
                self->features2c = f;
                break;
            case MGK_MATCHES:
                m = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
                navlcm_feature_match_set_t_read (m, fp);
                self->matches = m;
                break;
            case MGK_GMATCHES:
                m = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
                navlcm_feature_match_set_t_read (m, fp);
                self->gmatches = m;
                break;
            case MGK_MISMATCHES:
                m = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
                navlcm_feature_match_set_t_read (m, fp);
                self->mismatches = m;
                break;
            default:
                fprintf (stderr, "error reading data file. code = %c\n", code);
                break;
        }
    }

    if (self->features1)
        self->features_param->feature_type = self->features1->feature_type;
    if (self->features2)
        self->features_param->feature_type = self->features2->feature_type;

    fclose (fp);

}

void clear_matches (state_t *self)
{
    // clear matches
    if (self->matches)
        navlcm_feature_match_set_t_destroy (self->matches);
    self->matches = NULL;
   
    if (self->mismatches)
        navlcm_feature_match_set_t_destroy (self->mismatches);
    self->mismatches = NULL;

    self->highlight_m = NULL;
    self->select_m = NULL;
}

void clear_features (state_t *self, gboolean frame1, gboolean frame2)
{
    // clear matches
    clear_matches (self);

    if (frame1) {
        // clear features
        if (self->features1)
            navlcm_feature_list_t_destroy (self->features1);
        self->features1 = NULL;
        if (self->features1c)
            navlcm_feature_list_t_destroy (self->features1c);
        self->features1c = NULL;

    }

    if (frame2) {
        // clear features
        if (self->features2)
            navlcm_feature_list_t_destroy (self->features2);
        self->features2 = NULL;
        if (self->features2c)
            navlcm_feature_list_t_destroy (self->features2c);
        self->features2c = NULL;

    }

    self->highlight_f = NULL;
    self->match_f1 = NULL;
    self->match_f2 = NULL;

}
void clear_data (state_t *self, gboolean frame1, gboolean frame2)
{

    clear_features (self, frame1, frame2);

    if (frame1) {
        
        // clear images
        for (int i=0;i<NSENSORS;i++) {
            if (self->frame1[i])
                botlcm_image_t_destroy (self->frame1[i]);
            self->frame1[i] = NULL;
        }
    }

    if (frame2) {

        // clear images
        for (int i=0;i<NSENSORS;i++) {
            if (self->frame2[i])
                botlcm_image_t_destroy (self->frame2[i]);
            self->frame2[i] = NULL;
        }
    }

}

/* save a copy of features
 */
void duplicate_features (state_t *self)
{
    if (self->features1c)
        navlcm_feature_list_t_destroy (self->features1c);
    self->features1c = navlcm_feature_list_t_copy (self->features1);
    if (self->features2c)
        navlcm_feature_list_t_destroy (self->features2c);
    self->features2c = navlcm_feature_list_t_copy (self->features2);

}

/* tests whether two matches are equal, assuming that features have a unique index
 */
gboolean match_equal (navlcm_feature_match_t *m1, navlcm_feature_match_t *m2)
{
    if (!m1 || !m2) return false;
    return (m1->src.index == m2->src.index && m1->dst[0].index == m2->dst[0].index);
}

gboolean match_has (navlcm_feature_match_t *m, navlcm_feature_t *f)
{
    return (m->src.index == f->index);
}

/* find matches that are in set1 but not in set2
 */
navlcm_feature_match_set_t *util_subtract_sets (navlcm_feature_match_set_t *set1, navlcm_feature_match_set_t *set2)
{
    if (!set1 || !set2) return NULL;
    
    navlcm_feature_match_set_t *set = navlcm_feature_match_set_t_create ();

    for (int i=0;i<set1->num;i++) {
        navlcm_feature_match_t *m1 = set1->el + i;

        gboolean found = FALSE;
        for (int j=0;j<set2->num;j++) {
            navlcm_feature_match_t *m2 = set2->el + j;
            if (match_equal (m1, m2)) {
                found = TRUE;
                break;
            }
        }

        if (!found) {
            navlcm_feature_match_t *copy = navlcm_feature_match_t_copy (m1);
            navlcm_feature_match_set_t_insert (set, copy);
            free (copy);
        }
    }

    dbg (DBG_INFO, "found %d mismatches (set 1: %d matches, set 2: %d matches.", set->num, set1->num, set2->num);

    return set;
}


/* compute precision (set2 is ground-truth)
 * i.e. how many matches from set1 are in set2?
 */
double compute_precision (navlcm_feature_match_set_t *set1, navlcm_feature_match_set_t *set2)
{
    int count=0;

    if (!set1 || !set2) return .0;
    
    for (int i=0;i<set1->num;i++) {
        navlcm_feature_match_t *m1 = set1->el + i;

        gboolean found = FALSE;
        for (int j=0;j<set2->num;j++) {
            navlcm_feature_match_t *m2 = set2->el + j;
            if (match_equal (m1, m2)) {
                found = TRUE;
                break;
            }
        }

        if (found) count++;
    }

    if (set1->num > 0)
        return 1.0 * count / set1->num;
    else
        return 1.0;
}

/* compute recall (set2 is ground-truth)
 * i.e. how many matches from set2 have really been found in set1?
 */
double compute_recall (navlcm_feature_match_set_t *set1, navlcm_feature_match_set_t *set2)
{
    return compute_precision (set2, set1);
}

/* select a keypoint
 */
void select_keypoint (state_t *self)
{
    if (!self->highlight_f) return;

    if (navlcm_feature_list_t_search (self->features1c, self->highlight_f)) {
        self->match_f1 = self->highlight_f;
        return;
    }

    if (navlcm_feature_list_t_search (self->features2c, self->highlight_f)) {
        self->match_f2 = self->highlight_f;
    }
    gtku_gl_drawing_area_invalidate (self->gl_area_1);
}

/* select a match
 */
void select_match (state_t *self)
{
    if (self->highlight_m)
        self->select_m = self->highlight_m;
}

/* delete a keypoint
 */
gboolean delete_keypoint (state_t *self)
{
    gboolean ok = false;

    if (self->match_f1) {

        // remove potential match first
        remove_match (self, self->match_f1);

        // add to history
        history_push (self, HISTORY_DELETE_KEYPOINT, navlcm_feature_t_copy (self->match_f1));
        
        // delete keypoint
        navlcm_feature_list_t_remove (self->features1c, self->match_f1);
        self->match_f1 = NULL;

        // consequently, unselect the second keypoint as well
        self->match_f2 = NULL;
        self->highlight_f = NULL;

        ok = true;

    }
    gtku_gl_drawing_area_invalidate (self->gl_area_1);
    return ok;
}

/* delete a match
 */
gboolean delete_match (state_t *self)
{
    gboolean ok = false;

    if (self->select_m) {

        // add to history
        history_push (self, HISTORY_DELETE_MATCH, navlcm_feature_match_t_copy (self->select_m));

        // delete match
        if (self->gmatches)
            navlcm_feature_match_set_t_remove_copy (self->gmatches, self->select_m);
        self->select_m = NULL;
        self->highlight_m = NULL;
        self->highlight_f = NULL;

        ok = true;
    }
    gtku_gl_drawing_area_invalidate (self->gl_area_1);
    return ok;
}

/* add a keypoint
 */
void add_keypoint (state_t *self, navlcm_feature_t *f)
{
    if (self->features1c) {
        navlcm_feature_list_t_insert_head (self->features1c, f);
    }
}

/* add a match
 */
void add_match (state_t *self)
{
    if (self->match_f1 && self->match_f2) {
        if (!self->gmatches) 
            self->gmatches = navlcm_feature_match_set_t_create ();
        navlcm_feature_match_t *m = navlcm_feature_match_t_create (self->match_f1);
        double dist = vect_sqdist_float (self->match_f1->data, self->match_f2->data, self->match_f1->size);
        navlcm_feature_t *copy = navlcm_feature_t_copy (self->match_f2);
        navlcm_feature_match_t_insert (m, copy, dist);
        free (copy);

        if (!navlcm_feature_match_set_t_search_copy (self->gmatches, m)) {
            // add to history
            history_push (self, HISTORY_ADD_MATCH, navlcm_feature_match_t_copy (m));
            // add match to ground-truth matches
            navlcm_feature_match_t *mcopy = navlcm_feature_match_t_copy (m);
            navlcm_feature_match_set_t_insert (self->gmatches, mcopy);
            free (mcopy);
        }
        navlcm_feature_match_t_destroy (m);

        // remove feature from copy
        navlcm_feature_list_t_remove (self->features1c, self->match_f1);
        navlcm_feature_list_t_remove (self->features2c, self->match_f2);
        self->match_f1 = NULL;
        self->match_f2 = NULL;
        self->highlight_f = NULL;
    }

}

/* add a match
 */
void add_match (state_t *self, navlcm_feature_match_t *match)
{
    if (!self->gmatches) return;

    if (match) {
        // add to history
        history_push (self, HISTORY_ADD_MATCH, navlcm_feature_match_t_copy (match));
        // add match to matches
        navlcm_feature_match_t *copy = navlcm_feature_match_t_copy (match);
        navlcm_feature_match_set_t_insert (self->gmatches, copy);
        free (copy);
        // remove feature from copy
        navlcm_feature_t *c1 = navlcm_feature_t_find_by_index (self->features1c, match->src.index);
        navlcm_feature_list_t_remove (self->features1c, c1);
        navlcm_feature_t *c2 = navlcm_feature_t_find_by_index (self->features2c, match->dst[0].index);
        navlcm_feature_list_t_remove (self->features2c, c2);
    }
}

/* replace match
*/
void replace_match (state_t *self, navlcm_feature_match_t *match)
{
    if (!self->gmatches) return;

    // remove existing match
    for (int i=0;i<self->gmatches->num;i++) {
        navlcm_feature_match_t *m = self->gmatches->el + i;
        if (m->src.index == match->src.index) {
            remove_match (self, &m->src);
            break;
        }
    }

    // insert
    add_match (self, match);
}

/* remove match
*/
void remove_match (state_t *self, navlcm_feature_t *f)
{
    if (!self->gmatches) return;

    for (int i=0;i<self->gmatches->num;i++) {
        navlcm_feature_match_t *m = self->gmatches->el + i;
        if (match_has (m, f)) {
            navlcm_feature_match_set_t_remove (self->gmatches, m);
            break;
        }
    }
}

void util_compute_mismatches (state_t *self)
{
    if (self->mismatches)
        navlcm_feature_match_set_t_destroy (self->mismatches);
    self->mismatches = util_subtract_sets (self->matches, self->gmatches);
}

struct history_t { int code; gpointer data;};

void history_push (state_t *self, int code, gpointer data)
{
    history_t *h = (history_t*)malloc(sizeof(history_t));
    h->code = code;
    h->data = data;

    g_queue_push_head (self->history, h);
}

void history_pop (state_t *self)
{
    navlcm_feature_t *f = NULL;
    navlcm_feature_t *m = NULL;

    if (g_queue_is_empty (self->history)) return;

    history_t *h = (history_t*)g_queue_pop_head (self->history);

    switch (h->code) {
        case HISTORY_DELETE_KEYPOINT:
            add_keypoint (self, (navlcm_feature_t*)h->data);
            break;
        case HISTORY_ADD_MATCH:
            if (self->gmatches)
                navlcm_feature_match_set_t_remove_copy (self->gmatches, (navlcm_feature_match_t*)h->data);
            break;
        case HISTORY_DELETE_MATCH:
            if (self->gmatches)
                navlcm_feature_match_set_t_insert (self->gmatches, (navlcm_feature_match_t*)h->data);
            break;
        default:
            fprintf (stderr, "unrecognized history code %d\n", h->code);
            break;
    }
    free (h);

    gtku_gl_drawing_area_invalidate (self->gl_area_1);

}

void util_on_key_release (state_t *self, int keyval)
{
    if (keyval == GDK_f) {
        self->feature_finder_pressed = FALSE;
    }
}

void util_on_key_press (state_t *self, int keyval)
{
    if (keyval == GDK_Escape) {
        self->select_m = NULL;
        self->match_f1 = NULL;
        self->match_f2 = NULL;
    }

    if (keyval == GDK_Delete) {
        if (!delete_match (self))
            delete_keypoint (self);
        util_compute_mismatches (self);
    }

    if (keyval == GDK_u) {
        history_pop (self);
    }

    if (keyval == GDK_i) {
        if (self->select_m) {
            replace_match (self, self->select_m);
            util_compute_mismatches (self);
            self->select_m = NULL;
            self->highlight_m = NULL;
        }
    }

    // 'Enter' key
    if (keyval == 65293) {
        // check whether a match is being validated
        add_match (self);
        util_compute_mismatches (self);
    }

    if (keyval == GDK_f) {
        self->feature_finder_pressed = TRUE;
    }

}

void util_on_button_press (state_t *self, int button)
{
    if (button == 3) {
        // reset scale and translation
        self->user_scale_1 = 1.0;
        self->user_trans_1[0] = .0;
        self->user_trans_1[1] = .0;
        gtku_gl_drawing_area_invalidate (self->gl_area_1);
    }

    if (button == 1) {
        // select keypoint
        if (self->highlight_f) {
            select_keypoint (self);
            self->select_m = NULL;
        } else {
            // or select match
            if (self->highlight_m) {
                self->match_f1 = NULL;
                self->match_f2 = NULL;
                select_match (self);
            }
        }
    }
}

void util_span_matching_threshold (state_t *self)
{
    // determine matching threshold range and current value
    GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (glade_xml_get_widget(self->gladexml, "match_thresh")));
    double minval = 0.0;//gtk_adjustment_get_lower (adj);
    double maxval = 1.0;//gtk_adjustment_get_upper (adj);
    double curval = gtk_range_get_value (GTK_RANGE (glade_xml_get_widget(self->gladexml, "match_thresh")));
    int steps = 50;

    // determine filename
    char filename[256];
    int filecount=0;
    sprintf (filename, "precision-recall_%d.dat", filecount);
    while (file_exists (filename)) {
        filecount++;
        sprintf (filename, "precision-recall_%d.dat", filecount);
    }

    FILE *fp = fopen (filename, "w");
    int n=0;

    for (double val=minval;val<=maxval;val+=(maxval-minval)/steps) {
        printf ("step val = %.4f\n", val);

        // set threshold (recompute matches automatically)
        gtk_range_set_value (GTK_RANGE (glade_xml_get_widget(self->gladexml, "match_thresh")), val);

        if (self->matches->num == 0) continue;

        // compute precision and recall
        double precision = compute_precision (self->matches, self->gmatches);
        double recall = compute_recall (self->matches, self->gmatches);
        double mean = 2 * precision * recall / (precision + recall); // F-measure = harmonic mean
        fprintf (fp, "%d %.4f %.4f %.4f %.4f %.4f %d\n", n, val, recall, precision, 1.0 - precision, mean, self->matches->num);
        n++;
    }

    fclose (fp);

    printf ("computed precision/recall for %d values.\n", n);

    // reset to original value
    gtk_range_set_value (GTK_RANGE (glade_xml_get_widget(self->gladexml, "match_thresh")), curval);
}

