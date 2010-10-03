#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <glade/glade.h>

#include <lcm/lcm.h>
#include <common/glib_util.h>

#include <bot/bot_core.h>
#include <bot/gtk/gtk_util.h>
#include <bot/gl/texture.h>
#include <bot/gl/gl_util.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <lcmtypes/navlcm_class_param_t.h>

#define GLADE_FILE GLADE_TARGET_PATH  "/label-tool.glade"

typedef struct {
    int64_t utime;
    int motion_type[2];
} user_motion_type_t;

typedef struct {
    lcm_t *lcm;
    int64_t utime;
    int64_t min_utime, max_utime;
    GladeXML *gladexml;
    BotGtkGlDrawingArea *gl_area;
    botlcm_image_t *img_left, *img_right;
    GQueue *user_motion_types;
    char *last_load_filename;
    char *last_save_filename;

} state_t;

const double COLORS[5][3] = { {1,0,1}, {1,0,0}, {0,1,0}, {1,1,0}, {0,1,1}};

gint user_motion_type_comp (gconstpointer a, gconstpointer b, gpointer user_data)
{
    const user_motion_type_t *pa = (const user_motion_type_t*)a;
    const user_motion_type_t *pb = (const user_motion_type_t*)b;
    if (pa->utime < pb->utime) return -1;
    if (pa->utime == pb->utime) return 0;
    return 1; 
}

    static void
on_botlcm_image (const lcm_recv_buf_t *buf, const char *channel, 
        const botlcm_image_t *msg, void *data)
{
    state_t *self = (state_t*)data;

    self->utime = msg->utime;

    time_t time = self->utime / 1000000;

    // update time bounds
    if (self->min_utime == 0 || self->utime < self->min_utime)
        self->min_utime = self->utime;

    if (self->max_utime == 0 || self->max_utime < self->utime)
        self->max_utime = self->utime;

    // update time label
    if (self->gladexml) {
        GtkWidget *logtime_label = glade_xml_get_widget (self->gladexml, "logtime_label");
        if (logtime_label) {
            char *strtime = ctime (&time);
            strtime[strlen(strtime)-1] = '\0'; // remove carriage return
            gtk_label_set_text (GTK_LABEL (logtime_label), strtime);
        }
    }

    if (strcmp (channel, "CAM_THUMB_FL") == 0) {
        if (self->img_left)
            botlcm_image_t_destroy (self->img_left);
        self->img_left = botlcm_image_t_copy (msg);
    }
    if (strcmp (channel, "CAM_THUMB_FR") == 0) {
        if (self->img_right)
            botlcm_image_t_destroy (self->img_right);
        self->img_right = botlcm_image_t_copy (msg);
    }

    bot_gtk_gl_drawing_area_invalidate (self->gl_area);
}

    static gboolean
on_gl_expose (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    state_t *self = (state_t*)data;

    // black background
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int width = widget->allocation.width;
    int height = widget->allocation.height;

    glViewport(0, 0, width, height);        // reset the viewport to new dimensions 
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f (1,1,1);

    if (self->img_left) {
        int c0 = MAX(0, width/2 - self->img_left->width);
        int r0 = 10;
        BotGlTexture *tex = bot_gl_texture_new (self->img_left->width, self->img_left->height, self->img_left->size);

        bot_gl_texture_upload (tex, GL_LUMINANCE, GL_UNSIGNED_BYTE, self->img_left->row_stride, self->img_left->data);

        bot_gl_texture_draw_coords (tex, c0, r0, c0, r0 + self->img_left->height, c0 + self->img_left->width, r0 + self->img_left->height,
                c0 + self->img_left->width, r0);

        bot_gl_texture_free (tex);
    }

    if (self->img_right) {
        int c0 = MAX(0, width/2 - self->img_right->width) + self->img_right->width + 1;
        int r0 = 10;
        BotGlTexture *tex = bot_gl_texture_new (self->img_right->width, self->img_right->height, self->img_right->size);

        bot_gl_texture_upload (tex, GL_LUMINANCE, GL_UNSIGNED_BYTE, self->img_right->row_stride, self->img_right->data);

        bot_gl_texture_draw_coords (tex, c0, r0, c0, r0 + self->img_right->height, c0 + self->img_right->width, r0 + self->img_right->height,
                c0 + self->img_right->width, r0);

        bot_gl_texture_free (tex);
    }

    if (self->min_utime != self->max_utime) {

        // display log duration
        int usecs = (int)(self->max_utime - self->min_utime);
        int secs = (int)(usecs/1000000);
        int mins = (int)(secs/60);
        int hours = (int)(mins/60);
        mins -= hours * 60;
        secs -= mins * 60;
        char strduration[128];
        sprintf (strduration, "%02d:%02d:%02d", hours, mins, secs);
        double pos[3] = {width - 50, 250, 0};
        bot_gl_draw_text(pos, GLUT_BITMAP_HELVETICA_12, strduration,
                BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
                BOT_GL_DRAW_TEXT_ANCHOR_TOP |
                BOT_GL_DRAW_TEXT_ANCHOR_LEFT |
                BOT_GL_DRAW_TEXT_DROP_SHADOW);

        int border_width = 10;
        int timeline_height = 30;
        int c0 = 0;
        int r0 = height - border_width - timeline_height;
        glColor3f (1,1,1);
        glLineWidth(1);
        glBegin (GL_LINE_LOOP);
        glVertex2f (c0 + border_width, r0);
        glVertex2f (c0 + width - border_width, r0);
        glVertex2f (c0 + width - border_width, r0 + timeline_height);
        glVertex2f (c0 + border_width, r0 + timeline_height);
        glEnd ();

        int timeline_width = width - 2 * border_width;

        // draw motion types
        for (GList *iter=g_queue_peek_head_link (self->user_motion_types);iter;iter=iter->next) {
            user_motion_type_t *t = (user_motion_type_t*)iter->data;
            int64_t utime1 = t->utime;
            int64_t utime2 = iter->next ? ((user_motion_type_t*)iter->next->data)->utime : self->utime;
            if (utime2 < utime1)
                utime2 = self->max_utime;
            int mark1 = (int)(1.0 * (utime1 - self->min_utime) / (self->max_utime - self->min_utime) * timeline_width);
            int mark2 = (int)(1.0 * (utime2 - self->min_utime) / (self->max_utime - self->min_utime) * timeline_width);
            if (t->motion_type[0] != -1) {
                glColor3f (COLORS[t->motion_type[0]][0], COLORS[t->motion_type[0]][1], COLORS[t->motion_type[0]][2]);
                glBegin (GL_QUADS);
                glVertex2f (c0 + border_width + mark1, r0 + 1);
                glVertex2f (c0 + border_width + mark2, r0 + 1);
                glVertex2f (c0 + border_width + mark2, r0 + timeline_height/2-1);
                glVertex2f (c0 + border_width + mark1, r0 + timeline_height/2-1);
                glEnd ();
            }
            if (t->motion_type[1] != -1) {
                glColor3f (COLORS[t->motion_type[1]][0], COLORS[t->motion_type[1]][1], COLORS[t->motion_type[1]][2]);
                glBegin (GL_QUADS);
                glVertex2f (c0 + border_width + mark1, r0 + timeline_height/2);
                glVertex2f (c0 + border_width + mark2, r0 + timeline_height/2);
                glVertex2f (c0 + border_width + mark2, r0 + timeline_height-1);
                glVertex2f (c0 + border_width + mark1, r0 + timeline_height-1);
                glEnd ();
            }
        }

        // draw marker
        int marker_position = (int)(1.0 * (self->utime - self->min_utime) / (self->max_utime - self->min_utime) * timeline_width);
        glLineWidth(3);
        glColor3f (1,1,1);
        glBegin (GL_LINES);
        glVertex2f (c0 + border_width + marker_position, r0);
        glVertex2f (c0 + border_width + marker_position, r0 + timeline_height);
        glEnd ();

    }

    bot_gtk_gl_drawing_area_swap_buffers (self->gl_area);

    return TRUE;
}

void clear_motion_types (GQueue *data)
{
    for (GList *iter=g_queue_peek_head_link (data);iter;iter=iter->next) {
        user_motion_type_t *t = (user_motion_type_t*)iter->data;
        free (t);
    }
    g_queue_clear (data);
}

void motion_types_random (GQueue *data, int maxval)
{
    srand (time (NULL));

    for (GList *iter=g_queue_peek_head_link (data);iter;iter=iter->next) {
        user_motion_type_t *t = (user_motion_type_t*)iter->data;
        t->motion_type[0] = MIN (maxval, MAX (0, (int)(1.0*rand()/RAND_MAX * maxval)));
        t->motion_type[1] = MIN (maxval, MAX (0, (int)(1.0*rand()/RAND_MAX * maxval)));
    }
}

GList *find_motion_type_by_utime (GQueue *data, int64_t utime)
{
    for (GList *iter=g_queue_peek_head_link (data);iter;iter=iter->next) {
        if (!iter->next) break;
        user_motion_type_t *t1 = (user_motion_type_t*)iter->data;
        user_motion_type_t *t2 = (user_motion_type_t*)iter->next->data;
        if (t1->utime <= utime && utime < t2->utime)
            return iter;
    }

    return NULL;

}

gboolean user_motion_type_equal (user_motion_type_t *a, user_motion_type_t *b)
{
    if (a->motion_type[0] == b->motion_type[0])
        return TRUE;
    return FALSE;
}

void compute_precision_recall (GQueue *ref_data, GQueue *data, double time_offset_sec)
{
    int total_precision = 0;
    int total_recall = 0;
    double precision = .0;
    double recall = .0;
    
    for (GList *iter=g_queue_peek_head_link (data);iter;iter=iter->next) {
        user_motion_type_t *t = (user_motion_type_t*)iter->data;
        GList *ptr = find_motion_type_by_utime (ref_data, t->utime + (int)(time_offset_sec*1000000.0));
        if (ptr) {
            user_motion_type_t *tr = (user_motion_type_t*)ptr->data;
            if (tr->motion_type[0] == -1)// || tr->motion_type[0] == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_FORWARD)
                continue;
            if (t->motion_type[0] != -1) {
                if (user_motion_type_equal (t, tr))
                    precision++;
                else
                    recall++;
                total_precision++;
            } else {
                recall++;
            }
            total_recall++;
        }
    }

    if (total_precision > 0)
        precision /= total_precision;

    if (total_recall > 0)
        recall = (total_recall - recall) / total_recall;

    printf ("Precision: %.1f %%  Recall: %.1f %%\n", precision*100.0, recall*100.0);
}

void on_clear_data (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;

    clear_motion_types (self->user_motion_types);

    bot_gtk_gl_drawing_area_invalidate (self->gl_area);

}

void load_data (state_t *self, char *filename, GQueue *motion_types)
{
    FILE *fp = fopen (filename, "rb");
    if (!fp) {
        fprintf (stderr, "failed to open file %s in rb mode.\n", filename);
        return;
    }

    int nitems = 0;

    while (1) {
        int64_t utime;
        int motion_type[2];

        if (fread (&utime, sizeof(int64_t), 1, fp) != 1) break; // end of file
        assert (fread (&motion_type[0], sizeof(int), 1, fp) == 1);
        assert (fread (&motion_type[1], sizeof(int), 1, fp) == 1);

        user_motion_type_t *t = (user_motion_type_t*)calloc (1, sizeof(user_motion_type_t));
        t->utime = utime;
        t->motion_type[0] = motion_type[0];
        t->motion_type[1] = motion_type[1];
        g_queue_push_tail (motion_types, t);
        g_queue_sort (motion_types, user_motion_type_comp, NULL);

        if (self->min_utime == 0 || utime < self->min_utime)
            self->min_utime = utime;

        if (self->max_utime == 0 || self->max_utime < utime)
            self->max_utime = utime;

        nitems++;
    }

    fclose (fp);

    printf ("read %d items from %s.\n", nitems, filename);

    time_t time;
    time = self->min_utime / 1000000;
    printf ("Start time: %s", ctime (&time));
    time = self->max_utime / 1000000;
    printf ("End time  : %s", ctime (&time));


}

void save_data (state_t *self, char *filename)
{
    FILE *fp = fopen (filename, "wb");
    if (!fp) {
        fprintf (stderr, "failed to open file %s in wb mode.\n", filename);
        return;
    }

    int nitems = 0;

    for (GList *iter=g_queue_peek_head_link (self->user_motion_types);iter;iter=iter->next) {
        user_motion_type_t *t = (user_motion_type_t*)iter->data;

        fwrite (&t->utime, sizeof(int64_t), 1, fp);
        fwrite (&t->motion_type[0], sizeof(int), 1, fp);
        fwrite (&t->motion_type[1], sizeof(int), 1, fp);

        nitems++;
    }

    fclose (fp);

    printf ("wrote %d items to %s.\n", nitems, filename);
}

void on_compare_timeline (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open Data File", NULL,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
            NULL);
    g_assert(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        GQueue *motion_types = g_queue_new ();

        // read data
        load_data (self, filename, motion_types);

        double time_offset = gtk_spin_button_get_value (GTK_SPIN_BUTTON (glade_xml_get_widget (self->gladexml, "time_offset_sec"))); 

        // compare timelines
        compute_precision_recall (self->user_motion_types, motion_types, time_offset);

        // compare with random
        motion_types_random (motion_types, 4);
        compute_precision_recall (self->user_motion_types, motion_types, time_offset);

        clear_motion_types (motion_types);

        g_queue_free (motion_types);
    }

    gtk_widget_destroy(dialog);
    bot_gtk_gl_drawing_area_invalidate (self->gl_area);

}

void on_load_data (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open Data File", NULL,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
            NULL);
    g_assert(dialog);
    if (self->last_load_filename)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                self->last_load_filename);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (self->last_load_filename)
            g_free (self->last_load_filename);
        self->last_load_filename = filename;

        // clear data
        on_clear_data (NULL, self);

        // read data
        load_data (self, filename, self->user_motion_types);
    }

    gtk_widget_destroy(dialog);
    bot_gtk_gl_drawing_area_invalidate (self->gl_area);

}

void on_save_data (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Data File", NULL,
            GTK_FILE_CHOOSER_ACTION_SAVE,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
            NULL);
    g_assert(dialog);
    if (self->last_save_filename)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                self->last_save_filename);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (self->last_save_filename)
            g_free (self->last_save_filename);
        self->last_save_filename = filename;

        // save data
        save_data (self, filename);
    }

    gtk_widget_destroy(dialog);
    bot_gtk_gl_drawing_area_invalidate (self->gl_area);

}

void on_set_motion_type (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;

    gint type1 = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget (self->gladexml, "motion_type_combobox")));
    gint type2 = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget (self->gladexml, "motion_type_combobox2")));

    if (type1 != -1) {
        // create and insert a new user motion type
        user_motion_type_t *t = (user_motion_type_t*)calloc (1, sizeof(user_motion_type_t));
        t->utime = self->utime;
        t->motion_type[0] = type1;
        t->motion_type[1] = type2 == 0 ? type1 : type2 - 1;
        g_queue_push_tail (self->user_motion_types, t);
        g_queue_sort (self->user_motion_types, user_motion_type_comp, NULL);
    }
    bot_gtk_gl_drawing_area_invalidate (self->gl_area);

}

void on_clear_current_motion_type (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;
    GList *current_item = NULL;

    for (GList *iter=g_queue_peek_head_link (self->user_motion_types);iter;iter=iter->next) {
        user_motion_type_t *t = (user_motion_type_t*)iter->data;
        if (self->utime < t->utime)
            break;
        current_item = iter;
    }

    if (current_item) {
        g_queue_remove (self->user_motion_types, current_item->data);
    }

    bot_gtk_gl_drawing_area_invalidate (self->gl_area);

}

void on_save_timeline_to_png (GtkButton *button, gpointer data)
{
    state_t *self = (state_t*)data;

    int width = 800;
    int height = width/15;
        
    double time_offset = gtk_spin_button_get_value (GTK_SPIN_BUTTON (glade_xml_get_widget (self->gladexml, "time_offset_sec"))); 

    IplImage *im = cvCreateImage (cvSize (width, height), 8, 3);

    int duration = (int)(self->max_utime - self->min_utime);

    printf ("Duration (sec.): %d\n", (int)(1.0*duration/1000000));

    cvRectangle (im, cvPoint (0,0), cvPoint (width-1, height-1), CV_RGB (255,255,255), CV_FILLED);
    for (GList *iter=g_queue_peek_head_link (self->user_motion_types);iter;iter=iter->next) {
        user_motion_type_t *t = (user_motion_type_t*)iter->data;
        int64_t utime1 = t->utime;
        int64_t utime2 = iter->next ? ((user_motion_type_t*)iter->next->data)->utime : self->max_utime;
        utime1 += (int)(time_offset * 1000000.0);
        utime2 += (int)(time_offset * 1000000.0);

        int mark1 = (int)(1.0 * (utime1 - self->min_utime) / (self->max_utime - self->min_utime) * width);
        int mark2 = (int)(1.0 * (utime2 - self->min_utime) / (self->max_utime - self->min_utime) * width);

        CvScalar color1, color2;
        if (t->motion_type[0] != -1)
            color1 = CV_RGB (COLORS[t->motion_type[0]][0]*255, COLORS[t->motion_type[0]][1]*255, COLORS[t->motion_type[0]][2]*255);
        else
            color1 = CV_RGB (0,0,0);

        if (t->motion_type[1] != -1)
            color2 = CV_RGB (COLORS[t->motion_type[1]][0]*255, COLORS[t->motion_type[1]][1]*255, COLORS[t->motion_type[1]][2]*255);
        else
            color2 = CV_RGB (0,0,0);


        cvRectangle (im, cvPoint (mark1, 0), cvPoint (mark2, height/2-1), color1, CV_FILLED);
        cvRectangle (im, cvPoint (mark1, height/2), cvPoint (mark2, height-1), color1, CV_FILLED);
    }

    const char *filename = "timeline.png";

    cvSaveImage (filename, im);

    cvReleaseImage (&im);

    printf ("Saved timeline to %d x %d image (%s)\n", width, height, filename);
}

int main (int argc, char *argv[])
{
    state_t *self = (state_t*)calloc(1,sizeof(state_t));
    self->lcm = lcm_create (NULL);
    if (!self->lcm)
        return 1;
    self->utime = 0;
    self->img_left = NULL;
    self->img_right = NULL;
    self->min_utime = 0;
    self->max_utime = 0;
    self->user_motion_types = g_queue_new ();
    self->last_load_filename = NULL;
    self->last_save_filename = NULL;

    gtk_init (&argc, &argv);

    glutInit(&argc, argv);

    self->gladexml = glade_xml_new (GLADE_FILE, NULL, NULL);
    if (!self->gladexml)
        return 1;

    GtkWidget *window = glade_xml_get_widget (self->gladexml, "window");
    if (!window)
        return 1;


    // video frame
    self->gl_area = BOT_GTK_GL_DRAWING_AREA (bot_gtk_gl_drawing_area_new (FALSE));

    GtkWidget *frame = glade_xml_get_widget (self->gladexml, "video_frame");
    if (frame)
        gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (self->gl_area));
    else
        return 1;

    g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (G_OBJECT (self->gl_area), "expose-event", G_CALLBACK (on_gl_expose), self);

    g_signal_connect (G_OBJECT (glade_xml_get_widget (self->gladexml, "clear_data_button")), "clicked", G_CALLBACK (on_clear_data), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget (self->gladexml, "save_data_button")), "clicked", G_CALLBACK (on_save_data), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget (self->gladexml, "load_data_button")), "clicked", G_CALLBACK (on_load_data), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget (self->gladexml, "set_motion_type_button")), "clicked", G_CALLBACK (on_set_motion_type), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget (self->gladexml, "clear_current_motion_type_button")), "clicked", G_CALLBACK (on_clear_current_motion_type), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget (self->gladexml, "save_timeline_to_png_button")), "clicked", G_CALLBACK (on_save_timeline_to_png), self);
    g_signal_connect (G_OBJECT (glade_xml_get_widget (self->gladexml, "compare_timeline_button")), "clicked", G_CALLBACK (on_compare_timeline), self);

    gtk_combo_box_set_active (GTK_COMBO_BOX (glade_xml_get_widget (self->gladexml, "motion_type_combobox")), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (glade_xml_get_widget (self->gladexml, "motion_type_combobox2")), 0);

    botlcm_image_t_subscribe (self->lcm, "CAM_THUMB.*", on_botlcm_image, self);

    //    gtk_widget_show (GTK_WIDGET (self->gl_area));
    gtk_widget_show_all (window);

    glib_mainloop_attach_lcm (self->lcm);

    gtk_main ();

    return 0;
}

