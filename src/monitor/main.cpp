/* 
 * This module performs a sanity check on LCM messages. In particular,
 * it ensures that all grabbers are working fine by monitoring the CAMLCM messages.
 *
 * This module was also designed to monitor hardware parameters (cpu, memory, etc.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>

/* From GLIB */
#include <glib.h>
#include <glib-object.h>

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_generic_cmd_t.h>
#include <bot/bot_core.h>

/* From common */
#include <common/lcm_util.h>
#include <common/getopt.h>
#include <common/dbg.h>
#include <common/globals.h>
#include <common/navconf.h>
#include <common/glib_util.h>
#include <common/ioutils.h>
#include <common/config_util.h>
#include <common/date.h>
#include <common/sysinfo.h>

struct state_t {
   
    int verbose;
    GMainLoop *loop;
    lcm_t *lcm;
    BotConf *config;
    navlcm_system_info_t *sysinfo;
};

static state_t *g_self;
    
int monitor_restart_system ()
{
    return system ("nv-reboot");
}

int monitor_shutdown_system ()
{
    return system ("sudo shutdown -P now");
}

int monitor_enable_sound (state_t *self)
{
    int status = system ("amixer set 'Master' 130");

    self->sysinfo->sound_enabled = 1;
}

int monitor_disable_sound (state_t *self)
{
    int status = system ("amixer set 'Master' 0");

    self->sysinfo->sound_enabled = 0;
}

/* timeout called to publish the classifier settings.
*/
gboolean publish_system_info (gpointer data)
{
    state_t *self = (state_t*)data;

    sysinfo_read_acpi (self->sysinfo);
    sysinfo_read_sys_cpu_mem (self->sysinfo);
    sysinfo_read_disk_stats  (self->sysinfo);

//    navlcm_system_info_t_print (self->sysinfo);

    navlcm_system_info_t_publish (self->lcm, "SYSTEM_INFO", self->sysinfo);

    return TRUE;
}

static void
on_system_cmd (const lcm_recv_buf_t *buf, const char *channel, 
        const navlcm_generic_cmd_t *msg, 
        void *user)
{
    state_t *self = g_self;

    if (msg->code == MONITOR_SOUND_PARAM_CHANGED) {
        int enabled;
        sscanf (msg->text, "%d", &enabled);
        if (enabled)
            monitor_enable_sound (self);
        else
            monitor_disable_sound (self);
    } else if (msg->code == MONITOR_SHUTDOWN_SYSTEM) {
        monitor_shutdown_system ();
    } else if (msg->code == MONITOR_RESTART_SYSTEM) {
        monitor_restart_system ();
    }

    // immediately publish to skip delays
    publish_system_info (self);
}

int main(int argc, char *argv[])
{
    g_type_init ();

    dbg_init ();

    // initialize application state and zero out memory
    g_self = (state_t*) calloc( 1, sizeof(state_t) );
    state_t *self = g_self;
    self->config = globals_get_config ();
    self->sysinfo = navlcm_system_info_t_create ();

    // run the main loop
    self->loop = g_main_loop_new(NULL, FALSE);

    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,     "Be verbose");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    self->verbose = getopt_get_bool(gopt, "verbose");

    self->lcm = lcm_create(NULL);
    if (!self->lcm)
        return 1;

    // attach lcm to main loop
    glib_mainloop_attach_lcm (self->lcm);

    navlcm_generic_cmd_t_subscribe (self->lcm, "SYSTEM_CMD", on_system_cmd, self);

    g_timeout_add_seconds (2, &publish_system_info, self);

    // disable sound by default
    monitor_disable_sound (self);

    // connect to kill signal
    signal_pipe_glib_quit_on_kill (self->loop);

    // run main loop
    g_main_loop_run (self->loop);

    // cleanup
    g_main_loop_unref (self->loop);

    return 0;
}
