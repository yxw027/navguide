#ifndef _EVENTLOG_H
#define _EVENTLOG_H

#include <stdio.h>
#include <stdint.h>

typedef struct
{
    FILE *f;
    int64_t eventcount;
} eventlog_t;


typedef struct
{
    int64_t eventnum, timestamp;
    int32_t channellen, datalen;

    char     *channel;
    char     *data;
} eventlog_event_t;

// mode must be "r" or "w"
eventlog_t      *eventlog_create(const char *path, const char *mode);

// read the next event; free the returned structure with log_free_event
eventlog_event_t *eventlog_read_next_event(eventlog_t *l);
void        eventlog_free_event(eventlog_event_t *le);

// eventnum will be filled in for you
int         eventlog_write_event(eventlog_t *l, eventlog_event_t *le);

// when you're done with the log, clean up after yourself!
void        eventlog_destroy(eventlog_t *l);

#endif
