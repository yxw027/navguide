/* 
 * This module allows to log messages. The LCM library provides a logger
 * but we had to write our own one to add more controls. This code is highly
 * similar to the original LCM logger.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>
#include <sys/statvfs.h>

#include <glib.h>

#include <lcm/lcm.h>
#include <lcmtypes/navlcm_logger_info_t.h>

#include <common/glib_util.h>
#include <common/dbg.h>
#include <common/date.h>
#include <common/lcm_util.h>
#include <common/getopt.h>
#include <common/config_util.h>
#include <common/fileio.h>

typedef struct logger logger_t;
struct logger
{
    lcm_eventlog_t *log;

    int64_t nevents;
    int64_t logsize;

    char *skip_channel;

    int64_t events_since_last_report;
    int64_t last_report_time;
    int64_t last_report_logsize;
    int64_t time0;
    char *fname;
    lcm_t    *lcm;
    double logfilerate_mbs;
    double logfilesize_mb;
};

struct state_t {
    gboolean record;
    GMainLoop *loop;
    logger_t logger;
    config_t *config;
};

static state_t *g_self;

/*
void beep_warning ()
{
    system ("beep -f 200 -l 2 -r 3");
}

void beep_warning_2 ()
{
    system ("beep -f 200 -l 2 -r 6");
}
*/

static inline int64_t timestamp_seconds(int64_t v)
{
    return v/1000000;
}

static inline int64_t timestamp_now()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

char *get_next_available_logname (state_t *self)
{
    int id = 0;
    char *name = (char*)malloc(1024*sizeof(char));
    struct stat buf;

    while (1) {
        memset (name, '\0', 1024);
        sprintf (name, "%s/%04d-%02d-%02d-%02d-%02d-%s-%03d.log", 
		self->config->data_dir, get_current_year (), 
                 get_current_month(),
                 get_current_day(), get_current_hour(),
		get_current_min(), getenv ("LOGNAME"), id);
        if (stat (name, &buf) < 0)
            break;
        id++;
    }

    return name;
}

void on_stop_logging (const lcm_recv_buf_t *rbuf, const char *channel, void *data)
{
    logger_t *logger = (logger_t*)data;
    if (logger->log) {
        dbg (DBG_INFO, "[logger] stopping log.");
        //publish_phone_msg (g_self->logger.lcm, "stopping log.");
        lcm_eventlog_destroy (logger->log);
        logger->log = NULL;
        free (logger->fname);
        logger->fname = NULL;
    }
}

void on_start_logging (const lcm_recv_buf_t *rbuf, const char *channel, void *data)
{
    logger_t *logger = (logger_t*)data;
    if (!logger->log) {
        char *logname = (char*)malloc(128);
        sprintf (logname, "lcmlog-%04d-%02d-%02d.%02d-%02d-%02d", get_current_year(), get_current_month(),
                get_current_day(), get_current_hour(), get_current_min(), get_current_sec());
        dbg (DBG_INFO, "[logger] logging to %s", logname);
        publish_phone_msg (logger->lcm, "starting to log...");
        logger->log = lcm_eventlog_create(logname, "w");
        logger->fname = logname;
        logger->logsize = 0;
    } 
}

void message_handler (const lcm_recv_buf_t *rbuf, const char *channel, void *u)
{
    logger_t *l = (logger_t*) u;

    if (l->skip_channel && strstr (channel, l->skip_channel))
        return;

    if (strcmp (channel, "START_LOGGING")==0 || strcmp (channel, "STOP_LOGGING")==0)
        return;

    // skip if no file is open
    if (!l->log) {
        return;
    } 

    lcm_eventlog_event_t  le;

    int64_t offset_utime = rbuf->recv_utime - l->time0;
    int channellen = strlen(channel);

    le.timestamp = rbuf->recv_utime;
    le.channellen = channellen;
    le.datalen = rbuf->data_size;
    // log_write_event will handle le.eventnum.

    le.channel = (char*)  channel;
    le.data = rbuf->data;

    lcm_eventlog_write_event(l->log, &le);

    l->nevents++;
    l->events_since_last_report ++;

    l->logsize += 4 + 8 + 8 + 4 + channellen + 4 + rbuf->data_size;

    if (offset_utime - l->last_report_time > 2000000) {
        double dt = (offset_utime - l->last_report_time)/1000000.0;

        double tps =  l->events_since_last_report / dt;
        double kbps = (l->logsize - l->last_report_logsize) / dt / 1024.0;

        dbg (DBG_INFO, "Summary: ti:%ld sec Events: %ld ( %ld MB )     TPS: %8.2f      KB/s: %8.2f",
                timestamp_seconds(offset_utime),
                l->nevents, l->logsize/1048576, 
                tps, kbps);

        // get available disk space and publish logger info
        struct statvfs vfs;
        if (statvfs (g_self->logger.fname, &vfs) == 0) {

            navlcm_logger_info_t info;
            l->logfilerate_mbs = kbps/1000.0; 
            l->logfilesize_mb = l->logsize/1048576;
            //info.availsize_mb = vfs.f_bsize * vfs.f_bfree/1048576;

            navlcm_logger_info_t_publish (g_self->logger.lcm, "LOGGER_INFO", &info);
        }

        publish_phone_msg (g_self->logger.lcm, "KB/s: %8.2f   ( %ld MB)", 
                kbps, l->logsize/1048576);
        /*
           printf("Summary: %s ti:%4"PRIi64"sec Events: %-9"PRIi64" ( %4"PRIi64" MB )      TPS: %8.2f       KB/s: %8.2f\n", 
           l->fname,
           timestamp_seconds(offset_utime),
           l->nevents, l->logsize/1048576, 
           tps, kbps);
           */
        l->last_report_time = offset_utime;
        l->events_since_last_report = 0;
        l->last_report_logsize = l->logsize;
    }
}

gboolean publish_logger_info (gpointer data)
{
    state_t *self = (state_t*)data;

    navlcm_logger_info_t info;

    if (self->logger.log) {
        info.logging = 1;
        info.logfilerate_mbs = self->logger.logfilerate_mbs;
        info.logfilesize_mb = self->logger.logfilesize_mb;
        info.logfilename = strdup (self->logger.fname);
    } else {
        info.logging = 0;
        info.logfilerate_mbs = 0;
        info.logfilesize_mb = 0;
        info.logfilename = strdup ("");
    }

    navlcm_logger_info_t_publish (self->logger.lcm, "LOGGER_INFO", &info);

    free (info.logfilename);

    return TRUE;
}

void sigint_handler (int signal) 
{
    // quit main loop
    if (g_main_loop_is_running (g_self->loop)) 
        g_main_loop_quit (g_self->loop);

}

int main(int argc, char *argv[])
{
    // set some defaults
    g_self = (state_t*)malloc(sizeof(state_t));
    state_t *self = g_self;

    memset (&self->logger, 0, sizeof (logger));
    self->logger.time0 = timestamp_now();
    self->logger.fname = NULL;
    self->logger.log = NULL;
    self->logger.logsize = 0;
    self->logger.skip_channel = NULL;

    getopt_t *gopt = getopt_create();

    getopt_add_bool   (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_bool   (gopt, 'f',   "force",    0,        "Start logging at startup");
    getopt_add_string (gopt, 's',   "skip",   "",       "Skip channels containing this string");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    // read config file
    if (!(self->config = read_config_file ())) {
        dbg (DBG_ERROR, "[viewer] failed to read config file.");
        return -1;
    }

    // check data dir
    if (!dir_exists (self->config->data_dir)) {
        dbg (DBG_ERROR, "Data dir %s does not exist.", self->config->data_dir);
        return 1;
    }

    if (strlen (getopt_get_string (gopt, "skip")) > 2) {
        self->logger.skip_channel = strdup (getopt_get_string (gopt, "skip"));
        printf ("Warning: skipping channels containing %s.\n", self->logger.skip_channel);
    }

    // begin logging
    // char *provider = "udpm://?recv_buf_size=2097152";

    self->logger.lcm = lcm_create (NULL);
    if (!self->logger.lcm) {
        fprintf (stderr, "Couldn't initialize LCM!");
        return -1;
    }

    if (getopt_get_bool (gopt, "force")) 
        on_start_logging (NULL, NULL, &self->logger);

    lcm_subscribe(self->logger.lcm, ".*", message_handler, &self->logger);
    lcm_subscribe(self->logger.lcm, "START_LOGGING", on_start_logging, &self->logger);
    lcm_subscribe(self->logger.lcm, "STOP_LOGGING", on_stop_logging, &self->logger);

    g_timeout_add_seconds (1, publish_logger_info, self);

    self->loop = g_main_loop_new (NULL, FALSE);
    signal_pipe_glib_quit_on_kill (self->loop);
    glib_mainloop_attach_lcm (self->logger.lcm);

    // main loop
    g_main_loop_run (self->loop);

    // cleanup
    glib_mainloop_detach_lcm (self->logger.lcm);
    lcm_destroy (self->logger.lcm);
    if (self->logger.log)
        lcm_eventlog_destroy (self->logger.log);
}
