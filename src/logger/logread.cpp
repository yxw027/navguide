#include "logread.h"

struct lcm_logread_t {
    lcm_t * lcm_out;

    char * filename;

    lcm_eventlog_t * log;
    lcm_eventlog_event_t * event;

    double speed;
    int64_t next_clock_time;

    int thread_created;
    GThread *timer_thread;
    int notify_pipe[2];
    int timer_pipe[2];
};

void
logread_destroy (lcm_logread_t *lr) 
{
    size_t sz;
    dbg (DBG_INFO, "closing lcm log read context\n");
    if (lr->thread_created) {
        /* Destroy the timer thread */
        int64_t abort_command = -1;
        sz = write (lr->timer_pipe[1], &abort_command, sizeof (abort_command));
        assert (sz>=0);
        g_thread_join (lr->timer_thread);
    }

    close (lr->notify_pipe[0]);
    close (lr->notify_pipe[1]);
    close (lr->timer_pipe[0]);
    close (lr->timer_pipe[1]);

    if (lr->event)
        lcm_eventlog_free_event (lr->event);
    if (lr->log)
        lcm_eventlog_destroy (lr->log);

    free (lr->filename);
    free (lr);
}

int64_t
timestamp_now (void)
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

void *
timer_thread (void * user)
{
    lcm_logread_t * lr = (lcm_logread_t*)user;
    int64_t abstime;
    size_t sz=0;

    while (read (lr->timer_pipe[0], &abstime, 8) == 8) {
        if (abstime < 0) return NULL;

        int64_t now = timestamp_now();

        if (abstime > now) {
            int64_t sleep_utime = abstime - now;
            struct timeval sleep_tv;
            sleep_tv.tv_sec = sleep_utime / 1000000;
            sleep_tv.tv_usec = sleep_utime % 1000000;

            // sleep until the next timed message, or until an abort message
            fd_set fds;
            FD_ZERO (&fds);
            FD_SET (lr->timer_pipe[0], &fds);

            int status = select (lr->timer_pipe[0] + 1, &fds, NULL, NULL, 
                    &sleep_tv);

            if (0 == status) {  
                // select timed out
                sz=write (lr->notify_pipe[1], "+", 1);
                assert (sz>=0);
            }
        } else {
            sz = write (lr->notify_pipe[1], "+", 1);
            assert (sz>=0);
       }
    }
    perror ("timer_thread read failed");
    return NULL;
}

void
new_argument (gpointer key, gpointer value, gpointer user)
{
    lcm_logread_t * lr = (lcm_logread_t*)user;
    if (!strcmp ((const char*)key, "speed")) {
        char *endptr = NULL;
        lr->speed = strtod ((const char*)value, &endptr);
        if (endptr == value)
            fprintf (stderr, "Warning: Invalid value for speed\n");
    }
}

int
load_next_event (lcm_logread_t * lr)
{
    if (lr->event)
        lcm_eventlog_free_event (lr->event);

    lr->event = lcm_eventlog_read_next_event (lr->log);
    if (!lr->event)
        return -1;

    return 0;
}

int
lcm_parse_url (const char * url, char ** provider, char ** target,
        GHashTable * args)
{
    if (!url || !strlen (url))
        return -1;

    char ** strs = g_strsplit (url, "://", 2);
    if (!strs[0] || !strs[1]) {
        g_strfreev (strs);
        return -1;
    }

    if (provider)
        *provider = strdup (strs[0]);
    if (target)
        *target = NULL;

    char ** strs2 = g_strsplit (strs[1], "?", 2);
    g_strfreev (strs);
    if (!strs2[0]) {
        g_strfreev (strs2);
        return 0;
    }

    if (strlen (strs2[0]) && target)
        *target = strdup (strs2[0]);

    if (!strs2[1] || !strlen (strs2[1]) || !args) {
        g_strfreev (strs2);
        return 0;
    }

    strs = g_strsplit_set (strs2[1], ",&", -1);
    g_strfreev (strs2);

    for (int i = 0; strs[i]; i++) {
        strs2 = g_strsplit (strs[i], "=", 2);
        if (strs2[0] && strlen (strs2[0])) {
            g_hash_table_insert (args, strdup (strs2[0]),
                                 strs2[1] ? strdup (strs2[1]) : strdup (""));
        }
        g_strfreev (strs2);
    }
    g_strfreev (strs);

    return 0;
}

int
logread_resume (lcm_logread_t *lr, int64_t utime)
{
    lr->event->timestamp = utime;
    lr->next_clock_time = -1;

    return 0;
}

int
logread_offset_next_clock_time (lcm_logread_t *lr, int64_t utime)
{
    if (lr) lr->next_clock_time += utime;

    return 0;
}

int64_t
logread_event_time (lcm_logread_t *lr)
{
    if (lr->event)
        return lr->event->timestamp;
    else
        return -1;
}

int
logread_seek_to_relative_time (lcm_logread_t *lr, int32_t offset)
{
    if (!lr || !lr->log || !lr->event)
        return -1;
    
    int64_t utime = lr->event->timestamp;
        
    /* Destroy the timer thread */
    int64_t abort_command = -1;
    size_t size = write (lr->timer_pipe[1], &abort_command, sizeof (abort_command));
    assert (size>=0);
    g_thread_join (lr->timer_thread);

    /* reset clocks */
    lr->next_clock_time = -1;
    lr->event->timestamp += offset*1000000;
    
    /* reset pipes */
    close (lr->notify_pipe[0]);
    close (lr->notify_pipe[1]);
    close (lr->timer_pipe[0]);
    close (lr->timer_pipe[1]);
    
    /* move to the right place in the file */
    lcm_eventlog_seek_to_timestamp (lr->log, utime + 1000000 * offset);
    
    size = pipe (lr->notify_pipe);
    assert (size>=0);
    size = pipe (lr->timer_pipe);
    assert (size>=0);
    
    /* restart thread */
    lr->timer_thread = g_thread_create (timer_thread, lr, TRUE, NULL);
    if (!lr->timer_thread) {
        fprintf (stderr, "Error: LCM failed to start timer thread\n");
        logread_destroy (lr);
        return 0;
    }
    size=write (lr->notify_pipe[1], "+", 1);
    assert (size>=0);

    return 0;
}


lcm_logread_t * 
logread_create (lcm_t * parent, const char *url)
{
    size_t sz=0;
    char * target = NULL;
    GHashTable * args = g_hash_table_new_full (g_str_hash, g_str_equal,
            free, free);
    if (lcm_parse_url (url, NULL, &target, args) < 0) {
        fprintf (stderr, "Error: Bad URL \"%s\"\n", url);
        g_hash_table_destroy (args);
        return NULL;
    }
    if (!target || !strlen (target)) {
        fprintf (stderr, "Error: Missing filename\n");
        g_hash_table_destroy (args);
        free (target);
        return NULL;
    }


    lcm_logread_t * lr = (lcm_logread_t*)calloc (1, sizeof (lcm_logread_t));
    lr->lcm_out = parent;
    lr->filename = target;
    lr->speed = 1;
    lr->next_clock_time = -1;

    g_hash_table_foreach (args, new_argument, lr);
    g_hash_table_destroy (args);

    dbg (DBG_INFO, "Initializing LCM log read context...\n");
    dbg (DBG_INFO, "Filename %s\n", lr->filename);

    sz = pipe (lr->notify_pipe);
    sz = pipe (lr->timer_pipe);
    assert (sz>=0);
    //fcntl (lcm->notify_pipe[1], F_SETFL, O_NONBLOCK);

    lr->log = lcm_eventlog_create (lr->filename, "r");
    if (!lr->log) {
        fprintf (stderr, "Error: Failed to open %s: %s\n", lr->filename,
                strerror (errno));
        logread_destroy (lr);
        return NULL;
    }

    if (load_next_event (lr) < 0) {
        fprintf (stderr, "Error: Failed to read first event from log\n");
        logread_destroy (lr);
        return NULL;
    }

    /* Start the reader thread */
    lr->timer_thread = g_thread_create (timer_thread, lr, TRUE, NULL);
    if (!lr->timer_thread) {
        fprintf (stderr, "Error: LCM failed to start timer thread\n");
        logread_destroy (lr);
        return NULL;
    }
    lr->thread_created = 1;

    sz = write (lr->notify_pipe[1], "+", 1);
    assert (sz>=0);
    return lr;
}

int 
logread_get_fileno (lcm_logread_t *lr)
{
    return lr->notify_pipe[0];
}

int
logread_handle (lcm_logread_t * lr)
{
    size_t sz=0;

    if (!lr->event) {
        dbg (DBG_ERROR, "[logread] event not initialized.");
        return -1;
    }

    char ch;
    int status = read (lr->notify_pipe[0], &ch, 1);
    if (status == 0) {
        fprintf (stderr, "Error: lcm_handle read 0 bytes from notify_pipe\n");
        return -1;
    }
    else if (status < 0) {
        fprintf (stderr, "Error: lcm_handle read: %s\n", strerror (errno));
        return -1;
    }

    int64_t now = timestamp_now ();
    /* Initialize the wall clock if this is the first time through */
    if (lr->next_clock_time < 0)
        lr->next_clock_time = now;

    /* publish the event */
    lcm_publish (lr->lcm_out, lr->event->channel, 
                 lr->event->data, lr->event->datalen);

    int64_t prev_log_time = lr->event->timestamp;
    if (load_next_event (lr) < 0)
        return 0; /* end-of-file */

    /* Compute the wall time for the next event */
    if (lr->speed > 0)
        lr->next_clock_time +=
            (lr->event->timestamp - prev_log_time) / lr->speed;
    else
        lr->next_clock_time = now;

    if (lr->next_clock_time > now)
        sz = write (lr->timer_pipe[1], &lr->next_clock_time, 8);
    else
        sz = write (lr->notify_pipe[1], "+", 1);

    assert (sz>=0);
    return 0;
}

