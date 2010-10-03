#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <gtk_util/gtk_util.h>

#include <common/glib_util.h>
#include <common/geometry.h>
#include <common/convexhull.h>

typedef struct _state_t {
    GLUquadricObj *quadric;

    GList *points;
    polygon2d_t *hull;
} state_t;

static gboolean
on_gl_expose (GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
    state_t * self = (state_t *) user_data;
    GtkuGLDrawingArea *gl_area = GTKU_GL_DRAWING_AREA (widget);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (0, widget->allocation.width, widget->allocation.height, 0, -1, 1);
    glMatrixMode (GL_MODELVIEW);

    // draw the hull, colored in
    glLineWidth (2.0);
    if (self->hull) {
        glColor3f (0, 1, 1);
        glBegin (GL_LINE_LOOP);
        for (int i=0; i<self->hull->npoints; i++) {
            glVertex2f (self->hull->points[i].x, 
                    self->hull->points[i].y);
        }
        glEnd ();
    }

    // draw the original points
    glColor3f (1, 0, 0);
    GList *piter;
    for (piter=self->points; piter; piter=piter->next) {
        point2d_t *point = (point2d_t*) piter->data;
        glPushMatrix ();
        glTranslatef (point->x, point->y, 0);
        gluDisk (self->quadric, 0, 5, 50, 1);
        glPopMatrix ();
    }

    // highlight the convex hull points
    if (self->hull) {
        glColor3f (1, 1, 1);
        for (int i=0; i<self->hull->npoints; i++) {
            glPushMatrix ();
            glTranslatef (self->hull->points[i].x, self->hull->points[i].y, 0);
            gluDisk (self->quadric, 0, 5, 50, 1);
            glPopMatrix ();
        }
    }

    // draw the original polygon
    glLineWidth (1.0);
    glColor3f (0, 0, 1);
    glBegin (GL_LINE_LOOP);
    for (piter=self->points; piter; piter=piter->next) {
        point2d_t *point = (point2d_t*) piter->data;
        glVertex2f (point->x, point->y);
    }
    glEnd ();

    gtku_gl_drawing_area_swap_buffers (gl_area);
    return TRUE;
}


static gboolean 
on_button_press (GtkWidget *widget, GdkEventButton *event, void *user_data)
{
    state_t *self = (state_t*) user_data;
    GtkuGLDrawingArea *gl_area = GTKU_GL_DRAWING_AREA (widget);

    if (event->button == 1) {
        double nx = event->x;
        double ny = event->y;
        self->points = g_list_append (self->points, point2d_new (nx, ny));

        if (self->hull) {
            free (self->hull);
            self->hull = NULL;
        }
        polygon2d_t *poly = polygon2d_new_from_glist (self->points);
//        self->hull = convexhull_simple_polygon_2d (poly);
        self->hull = convexhull_graham_scan_2d (poly);
        polygon2d_free (poly);
    } else if (event->button == 3) {
        gu_list_free_with_func (self->points, free);
        self->points = NULL;

        if (self->hull) {
            free (self->hull);
            self->hull = NULL;
        }
    }

    gtku_gl_drawing_area_invalidate (gl_area);
    return TRUE;
}

static void
setup_gtk (state_t *self)
{
    // create the main application window
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (window), "delete_event", gtk_main_quit, NULL);
    g_signal_connect (G_OBJECT (window), "destroy", gtk_main_quit, NULL);
    gtk_window_set_default_size (GTK_WINDOW (window), 600, 400);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    // OpenGL drawing area
    GtkWidget *gl_area = gtku_gl_drawing_area_new (TRUE);
    gtk_widget_set_events (gl_area, GDK_BUTTON_PRESS_MASK );
    gtk_container_add (GTK_CONTAINER (window), gl_area);
    gtk_widget_show (gl_area);

    // connect mouse and redraw signal handlers
    g_signal_connect (G_OBJECT (gl_area), "expose-event",
            G_CALLBACK (on_gl_expose), self);
    g_signal_connect (G_OBJECT (gl_area), "button-press-event",
            G_CALLBACK (on_button_press), self);

    gtk_widget_show (window);
}

int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    state_t *self = (state_t*)calloc (1, sizeof (state_t));

    setup_gtk (self);

    glClearColor (0,0,0,0);
    self->quadric = gluNewQuadric ();

    gtk_main ();

    // cleanup
    gu_list_free_with_func (self->points, free);
    if (self->hull) {
        polygon2d_free (self->hull);
    }
    gluDeleteQuadric (self->quadric);

    return 0;
}
