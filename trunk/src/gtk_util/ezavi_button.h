#ifndef __ezavi_button_h__
#define __ezavi_button_h__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <common/ezavi.h>

G_BEGIN_DECLS

#define EZAVI_BUTTON_TYPE  ezavi_button_get_type()
#define EZAVI_BUTTON(obj)  (G_TYPE_CHECK_INSTANCE_CAST( (obj), \
        EZAVI_BUTTON_TYPE, ezavi_button))
#define EZAVI_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
            EZAVI_BUTTON_TYPE, ezavi_button_class ))
#define IS_EZAVI_BUTTON(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            EZAVI_BUTTON_TYPE ))
#define IS_EZAVI_BUTTON_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE( \
            (klass), EZAVI_BUTTON_TYPE))

typedef struct _ezavi_button ezavi_button;
typedef struct _ezavi_button_class ezavi_button_class;

typedef void (*ezavi_callback_t) (ezavi_button *widget, 
        uint8_t *buffer, void *user_data);

struct _ezavi_button
{
    GtkToggleButton button;

    /* private */
    int recording;
    uint8_t *buf;
    void *user_data;

    ezavi_t *ezavi;
    ezavi_params_t ezavi_opts;
    double sample_rate;

    ezavi_callback_t callback;

    guint timer_id;
};

struct _ezavi_button_class
{
    GtkToggleButtonClass parent_class;
};

GType        ezavi_button_get_type(void);

/* creates a new ezavi_button.  The button will not work until both
 * ezavi_button_set_avi_params and ezavi_button_set_callback have been
 * called with valid values.
 */
ezavi_button *  ezavi_button_new(void);

// file_prefix      generated files will be prefixed with this name
// width, height    size the input image and recorded AVI
// sample_rate      times/second to invoke the callback
// frame_rate       frame/second of the generated AVI file
//
// calling this function automatically stops any recording that's currently
// taking place
void ezavi_button_set_avi_params( ezavi_button *eb,
        const char *file_prefix,
        int width, int height, 
        double sample_rate,
        double frame_rate );

// registers a callback function that is responsible for drawing on an RGB
// buffer provided by ezavi_button.  This function is called sample_rate times
// per second.
void ezavi_button_set_callback( ezavi_button *eb, ezavi_callback_t func,
       void *user_data );

G_END_DECLS

#endif  /* __ezavi_button_h__ */
