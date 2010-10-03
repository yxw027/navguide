#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <glib.h>

#include <bot/bot_core.h>
#include "globals.h"

#include "navconf.h"

//#define dbg(...) fprintf(stderr, __VA_ARGS__)
#define dbg(...) 

#define err(...) fprintf(stderr, __VA_ARGS__)

#define MAX_REFERENCES 10 //((1ULL << 60))

static lcm_t *global_lcm;
static int64_t global_lcm_refcount;

static GStaticRecMutex _mutex = G_STATIC_REC_MUTEX_INIT;

lcm_t * 
globals_get_lcm (void)
{
    g_static_rec_mutex_lock (&_mutex);
    if (global_lcm_refcount == 0) {
        assert (! global_lcm);

        global_lcm = lcm_create(NULL);
        if (! global_lcm) {
            fprintf (stderr, "failed to create LCM.\n");
            g_static_rec_mutex_unlock (&_mutex);
            return NULL;
        }
        bot_glib_mainloop_attach_lcm (global_lcm);
    }

    assert (global_lcm);

    if (global_lcm_refcount < (int)MAX_REFERENCES) global_lcm_refcount++;
    lcm_t *result = global_lcm;
    g_static_rec_mutex_unlock (&_mutex);
    return result;
}

    void 
globals_release_lcm (lcm_t *lcm)
{
    g_static_rec_mutex_lock (&_mutex);
    if (global_lcm_refcount == 0) {
        fprintf (stderr, "ERROR: singleton LC refcount already zero!\n");
        g_static_rec_mutex_unlock (&_mutex);
        return;
    }
    if (lcm != global_lcm) {
        fprintf (stderr, "ERROR: %p is not the singleton LC (%p)\n",
                lcm, global_lcm);
    }
    global_lcm_refcount--;

    if (global_lcm_refcount == 0) {
        bot_glib_mainloop_detach_lcm (global_lcm);
        lcm_destroy (global_lcm);
        global_lcm = NULL;
    }
    g_static_rec_mutex_unlock (&_mutex);
}

static BotConf *global_config;
static int64_t global_config_refcount;

    BotConf *
globals_get_config (void)
{
    g_static_rec_mutex_lock (&_mutex);

    if (global_config_refcount == 0) {
        assert (! global_config);

        global_config = navconf_parse_default ();
        if (! global_config) {
            g_static_rec_mutex_unlock (&_mutex);
            return NULL;
        }
    }
    assert (global_config);

    if (global_config_refcount < (int)MAX_REFERENCES) global_config_refcount++;
    BotConf *result = global_config;
    g_static_rec_mutex_unlock (&_mutex);
    return result;
}

    void
globals_release_config (BotConf *config)
{
    g_static_rec_mutex_lock (&_mutex);
    if (global_config_refcount == 0) {
        fprintf (stderr, "ERROR: singleton config refcount already zero!\n");
        g_static_rec_mutex_unlock (&_mutex);
        return;
    }
    if (config != global_config) {
        fprintf (stderr, "ERROR: %p is not the singleton BotConf (%p)\n",
                config, global_config);
    }
    global_config_refcount--;

    if (global_config_refcount == 0) {
        bot_conf_free (global_config);
        global_config = NULL;
    }
    g_static_rec_mutex_unlock (&_mutex);
}

static ATrans *global_atrans;
static int64_t global_atrans_refcount;

    ATrans *
globals_get_atrans (void)
{
    lcm_t *lcm = globals_get_lcm ();
    BotConf *config = globals_get_config ();

    g_static_rec_mutex_lock (&_mutex);

    if (global_atrans_refcount == 0) {
        assert (! global_atrans);

        global_atrans = atrans_new(lcm, config);
        if (! global_atrans) {
            g_static_rec_mutex_unlock (&_mutex);
            if (config)
                globals_release_config (config);
            if (lcm)
                globals_release_lcm (lcm);
            return NULL;
        }
    }
    assert (global_atrans);

    if (global_atrans_refcount < (int)MAX_REFERENCES) global_atrans_refcount++;
    ATrans *result = global_atrans;
    g_static_rec_mutex_unlock (&_mutex);
    return result;
}

    void
globals_release_atrans (ATrans *atrans)
{
    g_static_rec_mutex_lock (&_mutex);

    assert (global_lcm);
    assert (global_config);

    if (global_atrans_refcount == 0) {
        fprintf (stderr, "ERROR: singleton atrans refcount already zero!\n");
        g_static_rec_mutex_unlock (&_mutex);
        return;
    }
    if (atrans != global_atrans) {
        fprintf (stderr,
                "ERROR: %p is not the singleton ATrans (%p)\n",
                atrans, global_atrans);
    }
    global_atrans_refcount--;

    if (global_atrans_refcount == 0) {
        atrans_destroy (global_atrans);
        global_atrans = NULL;
    }
    g_static_rec_mutex_unlock (&_mutex);

    globals_release_lcm (global_lcm);
    globals_release_config (global_config);
}

static GHashTable *_lcmgl_hashtable;

    bot_lcmgl_t *
globals_get_lcmgl (const char *name, const int create_if_missing)
{
    g_static_rec_mutex_lock (&_mutex);

    if (!global_lcm_refcount)
        globals_get_lcm();

    if (!_lcmgl_hashtable) {
        _lcmgl_hashtable = g_hash_table_new (g_str_hash, g_str_equal);
    }

    bot_lcmgl_t *lcmgl = (bot_lcmgl_t*)g_hash_table_lookup (_lcmgl_hashtable, name);
    if (!lcmgl&&create_if_missing) {
        lcmgl = bot_lcmgl_init (global_lcm, name);
        g_hash_table_insert (_lcmgl_hashtable, strdup(name), lcmgl);
    }

    g_static_rec_mutex_unlock (&_mutex);
    return lcmgl;
}


    GHashTable *
globals_get_lcmgl_hashtable (void)
{
    return _lcmgl_hashtable;
}

#if 0
static GHashTable *_lcmgl_hashtable;

    bot_lcmgl_t *
globals_get_lcmgl (const char *name, const int create_if_missing)
{
    g_static_rec_mutex_lock (&_mutex);

    if (!global_lcm_refcount)
        globals_get_lcm();

    if (!_lcmgl_hashtable) {
        _lcmgl_hashtable = g_hash_table_new (g_str_hash, g_str_equal);
    }

    bot_lcmgl_t *lcmgl = g_hash_table_lookup (_lcmgl_hashtable, name);
    if (!lcmgl&&create_if_missing) {
        lcmgl = bot_lcmgl_init (global_lcm, name);
        g_hash_table_insert (_lcmgl_hashtable, strdup(name), lcmgl);
    }

    g_static_rec_mutex_unlock (&_mutex);
    return lcmgl;
}


    GHashTable *
globals_get_lcmgl_hashtable (void)
{
    return _lcmgl_hashtable;
}


#endif
