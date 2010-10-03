#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "ioutils.h"
#include "eventlog.h"

#define MAGIC ((int32_t) 0xEDA1DA01L)

eventlog_t *eventlog_create(const char *path, const char *mode)
{
    assert(!strcmp(mode, "r") || !strcmp(mode, "w"));

    eventlog_t *l = (eventlog_t*) calloc(1, sizeof(eventlog_t));

    l->f = fopen(path, mode);
    if (l->f == NULL) {
        free (l);
        return NULL;
    }

    l->eventcount = 0;

    return l;
}

void eventlog_destroy(eventlog_t *l)
{
    fclose(l->f);
    free(l);
}

eventlog_event_t *eventlog_read_next_event(eventlog_t *l)
{
    eventlog_event_t *le = 
        (eventlog_event_t*) calloc(1, sizeof(eventlog_event_t));
    
    int32_t magic = 0;
    int r;

    do {
        r = fgetc(l->f);
        if (r < 0) goto eof;
        magic = (magic << 8) | r;
    } while( magic != MAGIC );

    fread64(l->f, &le->eventnum);
    fread64(l->f, &le->timestamp);
    fread32(l->f, &le->channellen);
    fread32(l->f, &le->datalen);

    assert (le->channellen < 1000);
    assert (le->datalen < 65536);

    if (l->eventcount != le->eventnum) {
        printf ("Mismatch: eventcount %"PRId64" eventnum %"PRId64"\n", 
                l->eventcount, le->eventnum);
        printf ("file offset %"PRId64"\n", ftello (l->f));
        l->eventcount = le->eventnum;
    }

    le->channel = calloc(1, le->channellen+1);
    if (fread(le->channel, 1, le->channellen, l->f) != (size_t) le->channellen)
        goto eof;

    le->data = calloc(1, le->datalen+1);
    if (fread(le->data, 1, le->datalen, l->f) != (size_t) le->datalen)
        goto eof;
    
    l->eventcount++;

    return le;

eof:
    return NULL;
}

int eventlog_write_event(eventlog_t *l, eventlog_event_t *le)
{
    fwrite32(l->f, MAGIC);

    le->eventnum = l->eventcount;

    fwrite64(l->f, le->eventnum);
    fwrite64(l->f, le->timestamp);
    fwrite32(l->f, le->channellen);
    fwrite32(l->f, le->datalen);

    fwrite(le->channel, 1, le->channellen, l->f);
    fwrite(le->data, 1, le->datalen, l->f);

    l->eventcount++;

    return 0;
}

void eventlog_free_event(eventlog_event_t *le)
{
    if (le->data) free(le->data);
    if (le->channel) free(le->channel);
    memset(le,0,sizeof(eventlog_event_t));
    free(le);
}
