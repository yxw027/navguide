#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "lc_util.h"

//#define dbg(...) fprintf (stderr, __VA_ARGS__)
#define dbg(...)

static lc_t * global_lc = NULL;
static int global_lc_refcount = 0;

lc_t * 
lcu_singleton_get ()
{
    if (global_lc_refcount == 0) {
        assert (! global_lc);

        global_lc = lc_create ();
        if (! global_lc) { return NULL; }

        if (0 != lc_init (global_lc, NULL)) {
            fprintf (stderr, "ERROR: could not initialize singleton LC\n");
            lc_destroy (global_lc);
            return NULL;
        }

        lcu_glib_mainloop_attach_lc (global_lc);
    }

    assert (global_lc);

    global_lc_refcount++;
    return global_lc;
}

void 
lcu_singleton_release ( lc_t *lc )
{
    if (global_lc_refcount == 0) {
        fprintf (stderr, "ERROR: singleton LC refcount already zero!\n");
        return;
    }
    if (lc != global_lc) {
        fprintf (stderr, "ERROR: %p is not the singleton LC (%p)\n",
                lc, global_lc);
    }
    global_lc_refcount--;

    if (global_lc_refcount == 0) {
        lcu_glib_mainloop_detach_lc (global_lc);
        lc_destroy (global_lc);
        global_lc = NULL;
    }
}


static int
lc_message_ready (GIOChannel *source, GIOCondition cond, lc_t *lc)
{
    lc_handle (lc);
    return TRUE;
}

typedef struct {
    GIOChannel *ioc;
    guint sid;
    lc_t *lc;
} glib_attached_lc_t;

static GHashTable *lc_glib_sources = NULL;

int
lcu_glib_mainloop_attach_lc (lc_t *lc)
{
    if (!lc_glib_sources) {
        lc_glib_sources = g_hash_table_new (g_direct_hash, g_direct_equal);
    }

    if (g_hash_table_lookup (lc_glib_sources, lc)) {
        dbg ("LC %p already attached to mainloop\n", lc);
        return -1;
    }

    glib_attached_lc_t *galc = 
        (glib_attached_lc_t*) calloc (1, sizeof (glib_attached_lc_t));

    galc->ioc = g_io_channel_unix_new (lc_get_fileno (lc));
    galc->sid = g_io_add_watch (galc->ioc, G_IO_IN, (GIOFunc) lc_message_ready, 
            lc);
    galc->lc = lc;

    dbg ("inserted LC %p into glib mainloop\n", lc);
    g_hash_table_insert (lc_glib_sources, lc, galc);

    return 0;
}

int
lcu_glib_mainloop_detach_lc (lc_t *lc)
{
    if (!lc_glib_sources) {
        dbg ("no lc glib sources\n");
        return -1;
    }

    glib_attached_lc_t *galc = 
        (glib_attached_lc_t*) g_hash_table_lookup (lc_glib_sources, lc);

    if (!galc) {
        dbg ("couldn't find matching gaLC\n");
        return -1;
    }

    dbg ("detaching LC from glib\n");
    g_io_channel_unref (galc->ioc);
    g_source_remove (galc->sid);

    g_hash_table_remove (lc_glib_sources, lc);
    free (galc);

    if (g_hash_table_size (lc_glib_sources) == 0) {
        g_hash_table_destroy (lc_glib_sources);
        lc_glib_sources = NULL;
    }

    return 0;
}
