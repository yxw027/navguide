#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gtk/gtksignal.h>

#include "ezavi_button.h"

#define dbg(args...) fprintf(stderr, args)
//#undef dbg
//#define dbg(args...)

typedef struct _param_data {
    char *name;
    GtkWidget *widget;
} param_data_t;

static void ezavi_button_class_init( ezavi_button_class *klass );
static void ezavi_button_init( ezavi_button *eb );
static void ezavi_button_toggled( ezavi_button *eb, void *user_data );
static gboolean ezavi_timer_callback( ezavi_button *eb );
static void ezavi_finalize( GObject *obj );
static void ezavi_button_stop_recording( ezavi_button *eb );

GType
ezavi_button_get_type(void)
{
    static GType eb_type = 0;
    if (!eb_type) {
        static const GTypeInfo eb_info =
        {
            sizeof (ezavi_button_class),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ezavi_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (ezavi_button),
            0,
            (GInstanceInitFunc) ezavi_button_init,
        };

        eb_type = g_type_register_static (GTK_TYPE_TOGGLE_BUTTON, 
                "ezavi_button", &eb_info, 0);
    }
    return eb_type;
}

static void
ezavi_button_class_init (ezavi_button_class *klass)
{
    GObjectClass *gclass = G_OBJECT_CLASS( klass );
    gclass->finalize = ezavi_finalize;
}

static void
ezavi_button_init( ezavi_button *eb )
{
    dbg("ezavi_button: constructor invoked\n");

    g_signal_connect( G_OBJECT( eb ), "toggled",
            G_CALLBACK( ezavi_button_toggled ), NULL );

    eb->ezavi = NULL;
}

static void
ezavi_finalize( GObject *obj )
{
    dbg("ezavi_button: finalize\n");
    ezavi_button *eb = EZAVI_BUTTON( obj );
    if( eb->ezavi ) ezavi_destroy( eb->ezavi );
    if( eb->buf ) free( eb->buf );
}

ezavi_button *
ezavi_button_new( void )
{
    return EZAVI_BUTTON( g_object_new( EZAVI_BUTTON_TYPE, 
               "label", "_Record", 
               "use-underline", TRUE,
               NULL ) );
}

void 
ezavi_button_set_avi_params( ezavi_button *eb,
        const char *file_prefix,
        int width, int height, 
        double sample_rate,
        double frame_rate )
{
    if( eb->recording ) {
        dbg("ezavi_button: parameters adjusted while recording.  "
                "stopping recording...\n");
        ezavi_button_stop_recording( eb );
    }
    dbg("ezavi_button: setting AVI params [%s] <%d, %d> sample: %.2f "
            "fps: %.2f\n", 
            file_prefix, width, height, sample_rate, frame_rate );

    if( eb->ezavi ) {
        ezavi_destroy( eb->ezavi );
    }

    eb->ezavi_opts.file_prefix = (char*) file_prefix;
    eb->ezavi_opts.codec = "raw";
    eb->ezavi_opts.width = width;
    eb->ezavi_opts.height = height;
    eb->ezavi_opts.frame_rate = frame_rate;
    eb->ezavi_opts.split_point = 0;
    eb->sample_rate = sample_rate;

    if( eb->buf ) {
        free( eb->buf );
    }
    eb->buf = (uint8_t*) malloc( width*height*3 );

    if( eb->recording ) {
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( eb ), FALSE );
    }
}

void 
ezavi_button_set_callback( ezavi_button *eb, ezavi_callback_t func,
       void *user_data )
{
    eb->callback = func;
    eb->user_data = user_data;
}

static gboolean
ezavi_timer_callback( ezavi_button *eb )
{
    if( ! eb->recording ) return FALSE;
    eb->callback( eb, eb->buf, eb->user_data );
    ezavi_write_video( eb->ezavi, eb->buf );
    return TRUE;
}

static void
ezavi_button_stop_recording( ezavi_button *eb ) 
{
    gtk_button_set_label( GTK_BUTTON( eb ), "_Record" );

    if( eb->recording ) {
        ezavi_destroy( eb->ezavi );
        eb->ezavi = NULL;
    }

    eb->recording = 0;

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( eb ), FALSE );
}

static void
ezavi_button_toggled( ezavi_button *eb, void *user_data )
{
    gboolean active = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( eb ) );

    if( active ) {
        if( ! eb->callback ) {
            g_warning("ezavi_button: need to set a callback before "
                    "recording!\n");
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( eb ), FALSE );
            return;
        }
        if( ! eb->ezavi_opts.file_prefix ||
            eb->ezavi_opts.width <= 0 ||
            eb->ezavi_opts.height <= 0 || 
            eb->sample_rate <= 0 ||
            eb->ezavi_opts.frame_rate <= 0 ) {
            g_warning("ezavi_button: invalid parameters!\n");
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( eb ), FALSE );
            return;
        }

        gtk_button_set_label( GTK_BUTTON( eb ), "_Stop Recording" );

        // we shouldn't be recording, but check just to be sure..
        if( eb->ezavi ) {
            ezavi_destroy( eb->ezavi );
        }

        eb->ezavi = ezavi_new( &eb->ezavi_opts );

        if( ! eb->ezavi ) {
            g_warning("ezavi_button: couldn't initialize ezavi!\n");
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( eb ), FALSE );
        }

        dbg("ezavi_button: start recording\n");

        // set a timer
        eb->timer_id = g_timeout_add_full(1000,
                (int)( 1000 / eb->sample_rate ), 
               (GSourceFunc)  ezavi_timer_callback, 
                eb, NULL );

        eb->recording = 1;

    } else {
        dbg("ezavi_button: stop recording\n");
        ezavi_button_stop_recording( eb );
    }
}
