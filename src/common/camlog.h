#ifndef __camlog_h__
#define __camlog_h__

#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Camlog Camlog;

Camlog* camlog_new( const char *fname, const char *mode );
void camlog_destroy( Camlog *self );

const char * camlog_get_stream_id( Camlog *self );

int camlog_seek_to_frame( Camlog *self, int frameno );

int camlog_seek_to_offset( Camlog *self, int64_t offset, int *frameno );

int camlog_get_format( Camlog *self, 
        int *width, int *height, int *row_stride, 
        uint32_t *pixel_format );

int camlog_get_next_frame_timestamp( Camlog *self, 
        int64_t *timestamp );

/**
 * camlog_get_next_frame:
 *
 * Returns: the number of bytes read, or -1 on failure
 */
int camlog_get_next_frame( Camlog *self, uint8_t *buf, 
        int buf_size, uint64_t * source_id );

int camlog_count_frames( Camlog *self );

int camlog_write_stream_id( Camlog *self, const char *stream_id );

int camlog_write_frame( Camlog *self, int width, int height, int stride,
        int pixelformat, int64_t timestamp, int bus_timestamp,
        uint64_t source_uid,
        const uint8_t *data, int datalen, int64_t * file_offset );

// legacy
#define LOG_MARKER  0xEDED

typedef enum {
    LOG_TYPE_FRAME_DATA = 1,
    LOG_TYPE_FRAME_FORMAT = 2,
    LOG_TYPE_FRAME_TIMESTAMP = 3,
    LOG_TYPE_COMMENT = 4,
    LOG_TYPE_STREAM_ID = 5,
    LOG_TYPE_SOURCE_UID = 6,
} LogType;

static inline void
log_put_short (uint16_t val, FILE * f)
{
    uint16_t fval = htons (val);
    fwrite (&fval, 2, 1, f);
};

static inline void
log_put_long (uint32_t val, FILE * f)
{
    uint32_t fval = htonl (val);
    fwrite (&fval, 4, 1, f);
}

static inline void
log_put_uint64 (uint64_t val, FILE * f)
{
    uint8_t b[8] = {
        val >> 56,
        val >> 48,
        val >> 40,
        val >> 32,
        val >> 24,
        val >> 16,
        val >> 8,
        val,
    };
    fwrite (b, 1, 8, f);
}

static inline void
log_put_field (uint16_t type, uint32_t length, FILE * f)
{
    log_put_short (LOG_MARKER, f);
    log_put_short (type, f);
    log_put_long (length, f);
};

static inline int
log_get_short (uint16_t * val, FILE * f)
{
    if (fread (val, 2, 1, f) != 1)
        return -1;
    *val = ntohs (*val);
    return 0;
};

static inline int
log_get_long (uint32_t * val, FILE * f)
{
    if (fread (val, 4, 1, f) != 1)
        return -1;
    *val = ntohl (*val);
    return 0;
};

static inline int
log_get_uint64 (uint64_t * val, FILE * f)
{
    uint8_t b[8];
    if (fread (b, 1, 8, f) != 8)
        return -1;
    *val = ((uint64_t)b[0] << 56) |
        ((uint64_t)b[1] << 48) |
        ((uint64_t)b[2] << 40) |
        ((uint64_t)b[3] << 32) |
        ((uint64_t)b[4] << 24) |
        ((uint64_t)b[5] << 16) |
        ((uint64_t)b[6] << 8) |
        (uint64_t)b[7];
    return 0;
};

static inline int
log_get_field (uint16_t * type, uint32_t * length, FILE * f)
{
    uint16_t marker;
    if (log_get_short (&marker, f) < 0)
        return -1;
    if (marker != LOG_MARKER) {
        fprintf (stderr, "Error: marker not found when reading log\n");
        return -1;
    }
    if (log_get_short (type, f) < 0)
        return -1;
    if (log_get_long (length, f) < 0)
        return -1;
    return 0;
};

#ifdef __cplusplus
}
#endif

#endif
