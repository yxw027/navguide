#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <gtk/gtk.h>
#include "gtkgldrawingarea.h"
#include <math.h>

typedef struct _point2d {
    double x;
    double y;
} point2d_t;

typedef struct _state_t {
    GtkuGLDrawingArea *gl_area;
    GtkWidget *window;
    GLUquadricObj *quadric;
    GList *todraw;

    point2d_t last_mouse;
} state_t;

void gl_look_at (double eye[3], double center[3], double up[3])
{
    gluLookAt (eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);
}

void vect_norm (double *a, int n)
{
    double length = 0.0;
    for (int i=0;i<n;i++)
        length += a[i] * a[i];
    length = sqrt (length);
    if (length > 1E-6)
        for (int i=0;i<n;i++)
            a[i] = a[i] / length;
}

static gboolean
on_gl_expose (GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
    state_t *self = (state_t*)user_data;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable (GL_DEPTH_TEST);
   
    int w = widget->allocation.width;
    int h = widget->allocation.height;
    double aspect = 1.0 * w / h;
    double znear = 1.0;
    double zfar = 1000.0;
    double fovy = 90.0;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, aspect, znear, zfar);

    double eye[3], center[3], up[3];
    eye[0] = 16.6;
    eye[1] = -37.0;
    eye[2] = -29.0;

    center[0] = 0.0;
    center[1] = 0.0;
    center[2] = 0.0;

    up[0] = .84;
    up[1] = .11;
    up[2] = .54;
    vect_norm (up, 3);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gl_look_at(eye, center, up);

    glPolygonMode(GL_FRONT,GL_LINE);
    glEnable (GL_DEPTH_TEST);
    double axis_center[3] = {0,0,0};
    //draw_3d_axis (axis_center, 50.0);

    glColor3f(1,1,0);

    glBegin (GL_QUADS);
    glVertex3f (-10, -10, -10);
    glVertex3f (100, -10, -10);
    glVertex3f (100, 100, -10);
    glVertex3f (-10, 100, -10);
    glEnd();

    glColor3f (1,0,0);
    glBegin (GL_QUADS);
    glVertex3f (-10, -10, 10);
    glVertex3f (100, -10, 10);
    glVertex3f (100, 100, 10);
    glVertex3f (-10, 100, 10);
    glEnd();
    glFlush ();

    gtku_gl_drawing_area_swap_buffers(self->gl_area);
    return TRUE;
}

static gboolean 
on_button_press( GtkWidget *widget, GdkEventButton *event, void *user_data )
{
    state_t *self = (state_t*) user_data;

    if( event->button == 1 ) {
        point2d_t *newpoint = (point2d_t*) malloc( sizeof(point2d_t) );
        newpoint->x = event->x / widget->allocation.width;
        newpoint->y = event->y / widget->allocation.height;
        self->todraw = g_list_append( self->todraw, newpoint );
    } else if( event->button == 3 ) {
        GList *piter;
        for( piter=self->todraw; piter; piter=piter->next ) free(piter->data);
        g_list_free( self->todraw );
        self->todraw = NULL;
    }

    gtku_gl_drawing_area_invalidate (self->gl_area);
    return TRUE;
}

static gboolean
on_button_release( GtkWidget *widget, GdkEventButton *event, void *user_data )
{
//    state_t *self = (state_t*) user_data;
    g_print("button %d released\n", event->button);
    return TRUE;
}

/**
 * this function is called when the user moves the mouse pointer over the
 * opengl window.  Use the event->state field to determine whether a button is
 * pressed or not
 */
static gboolean 
on_motion_notify( GtkWidget *widget, GdkEventMotion *event, void *user_data )
{
    state_t *self = (state_t*) user_data;
    self->last_mouse.x = event->x / widget->allocation.width;
    self->last_mouse.y = event->y / widget->allocation.height;
    gtku_gl_drawing_area_invalidate (self->gl_area);
    return TRUE;
}

static void
setup_gtk( state_t *self )
{
    // create the main application window
    self->window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    g_signal_connect( G_OBJECT(self->window), "delete_event", 
            gtk_main_quit, NULL );
    g_signal_connect( G_OBJECT(self->window), "destroy", 
            gtk_main_quit, NULL );
    gtk_window_set_default_size(GTK_WINDOW(self->window), 600, 400);
    gtk_container_set_border_width( GTK_CONTAINER(self->window), 10 );

    // create the aspect area to maintain a 1:1 aspect ratio

    GtkWidget *aspect = gtk_aspect_frame_new( NULL, 0.5, 0.5, 1, FALSE );
    gtk_container_add(GTK_CONTAINER(self->window), aspect);
    gtk_widget_show(aspect);

    self->gl_area = GTKU_GL_DRAWING_AREA(gtku_gl_drawing_area_new(TRUE));
    gtk_widget_set_events(GTK_WIDGET(self->gl_area), 
            GDK_LEAVE_NOTIFY_MASK |
            GDK_BUTTON_PRESS_MASK | 
            GDK_BUTTON_RELEASE_MASK | 
            GDK_POINTER_MOTION_MASK );
    gtk_container_add (GTK_CONTAINER(aspect), GTK_WIDGET(self->gl_area));
    gtk_widget_show (GTK_WIDGET (self->gl_area));

    g_signal_connect(G_OBJECT(self->gl_area), "expose-event",
            G_CALLBACK(on_gl_expose), self);
    g_signal_connect(G_OBJECT(self->gl_area), "button-press-event",
            G_CALLBACK(on_button_press), self);
    g_signal_connect(G_OBJECT(self->gl_area), "button-release-event",
            G_CALLBACK(on_button_release), self);
    g_signal_connect(G_OBJECT(self->gl_area), "motion-notify-event",
            G_CALLBACK(on_motion_notify), self);

    gtk_widget_show( self->window );
}

static void
setup_opengl( state_t *self )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glClearColor(0,0,0,0);
    glOrtho( 0, 1, 1, 0, -1, 1 );
    self->quadric = gluNewQuadric();
    self->last_mouse.x = -1;
    self->last_mouse.y = -1;
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    state_t *self = (state_t*)calloc(1, sizeof(state_t));

    setup_gtk( self );

    setup_opengl( self );

    gtk_main();

    GList *piter;
    for( piter=self->todraw; piter; piter=piter->next ) free( piter->data );
    g_list_free( self->todraw );
    self->todraw = NULL;

    return 0;
}
