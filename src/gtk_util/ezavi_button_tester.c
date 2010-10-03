#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <gtk/gtk.h>

#include <common/draw.h>
#include <gtk_util/rgb_canvas.h>
#include <gtk_util/ezavi_button.h>

typedef struct _state {
    GtkWidget *canvas;
    ezavi_button *eb;
} state_t;

static gboolean 
on_canvas_motion( GtkuRgbCanvas *canvas, GdkEventMotion *event, 
        gpointer data )
{
    assert( canvas->buf );

    int x = event->x;
    int y = event->y;
    int r = 10;

    draw_ellipse_rgb( canvas->buf, x-r, y-r, x+r, y+r, 0x8000ff00, 1, 0,
            canvas->buf_width, canvas->buf_height, canvas->buf_stride );

    gtk_widget_queue_draw_area( GTK_WIDGET(canvas), x-r, y-r, 2*r, 2*r );

    return TRUE;
}

static void
on_canvas_configure( GtkWidget *canvas, GdkEventConfigure *event, 
       void *user_data )
{
    state_t *s = (state_t*) user_data;
    ezavi_button_set_avi_params( s->eb, "ezavi_button_test", 
            canvas->allocation.width,
            canvas->allocation.height,
            25,        // sample rate
            25         // avi frame rate
            );
}

static void
on_ezavi_request( ezavi_button *widget, uint8_t *buf, void *user_data )
{
    state_t *s = (state_t*) user_data;
    GtkuRgbCanvas *canvas = GTKU_RGB_CANVAS(s->canvas);
    int tocopy = canvas->buf_height * canvas->buf_stride;
    memcpy( buf, canvas->buf, tocopy );
}

int main(int argc, char **argv)
{
    printf("rgb_canvas test\n");

    state_t s;
    memset( &s, 0, sizeof(s) );

    GtkWidget *window;

    gtk_init( &argc, &argv );

    // main window
    window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    g_signal_connect( G_OBJECT(window), "delete_event", gtk_main_quit, NULL );
    g_signal_connect( G_OBJECT(window), "destroy", gtk_main_quit, NULL );
    gtk_container_set_border_width( GTK_CONTAINER(window), 10 );

    // vbox
    GtkWidget *vbox;
    vbox = gtk_vbox_new( FALSE, 5 );
    gtk_container_add( GTK_CONTAINER(window), vbox );

    // canvas
    s.canvas = GTK_WIDGET(gtku_rgb_canvas_new());

    g_signal_connect( G_OBJECT(s.canvas), "motion_notify_event", 
           G_CALLBACK(on_canvas_motion), &s );
    g_signal_connect( G_OBJECT(s.canvas), "configure_event",
            G_CALLBACK(on_canvas_configure), &s ); 

    gtk_widget_set_size_request(s.canvas, 200, 200);

    gtk_widget_set_events(s.canvas, GDK_LEAVE_NOTIFY_MASK
            | GDK_BUTTON_PRESS_MASK
            | GDK_BUTTON_RELEASE_MASK
            | GDK_POINTER_MOTION_MASK );

    gtk_box_pack_start( GTK_BOX(vbox), s.canvas, TRUE, TRUE, 0 );

    // button
    s.eb = ezavi_button_new();

    ezavi_button_set_callback( s.eb, on_ezavi_request, &s );

    gtk_box_pack_start( GTK_BOX(vbox), GTK_WIDGET( s.eb ), FALSE, TRUE, 0 );

    gtk_widget_show_all( window );
    gtk_main();

    return 0;
}
