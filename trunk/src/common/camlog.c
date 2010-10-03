#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include <inttypes.h>
#include "ppm.h"

#include "camlog.h"

//#define dbg(args...) fprintf(stderr, args)
#define dbg(args...)
#define err(args...) fprintf(stderr, args)

struct _Camlog {
    FILE *fp;
    char *mode;
    off_t file_size;
    char *stream_id;

    uint16_t width;
    uint16_t height;
    uint16_t row_stride;
    uint32_t pixel_format;
    uint32_t frame_spacing;
    uint64_t first_timestamp_offset;
};

static int log_get_frame_format (FILE * f, uint32_t * pf, uint16_t * width,
        uint16_t * height, uint16_t * stride, uint32_t * frame_spacing);
static int log_get_next_frame_time (FILE * f, uint32_t * sec, 
        uint32_t * usec, uint32_t * bus);
static int log_get_stream_id (FILE *f, char **stream_id, int *len);

Camlog* 
camlog_new( const char *fname, const char *mode )
{
    if( ! strcmp( mode, "r" ) && ! strcmp( mode, "w" ) ) {
        err("camlog: mode must be either 'w' or 'r'!!\n");
        return NULL;
    }

    struct stat statbuf;
    if( mode[0] == 'r' ) {
        if (stat (fname, &statbuf) < 0) {
            fprintf (stderr, "Error: Failed to stat %s\n", fname);
            return NULL;
        }
    }

    Camlog *self = (Camlog*) calloc(1, sizeof(Camlog));

    self->mode = strdup(mode);
    self->fp = fopen(fname, mode);
    if( ! self->fp ) {
        err("camlog: couldn't open [%s]\n", fname);
        perror("fopen");
        camlog_destroy( self );
        return NULL;
    }

    if( mode[0] == 'r' ) {
        self->file_size = statbuf.st_size;

        int status;
        status = log_get_frame_format( self->fp, &self->pixel_format,
                &self->width, &self->height, &self->row_stride,
                &self->frame_spacing ); 
        if ( status != 0 ) {
            err("camlog: Cannot parse %s as log file\n", fname);
            camlog_destroy( self );
            return NULL;
        }

        int sidsize;
        if (log_get_stream_id (self->fp, &self->stream_id, &sidsize) < 0) {
            err("camlog: couldn't retrieve stream ID\n");
            self->stream_id = NULL;
        } else {
            dbg("camlog: loaded stream id [%s]\n", self->stream_id);
        }
        rewind(self->fp);

        rewind (self->fp);
        uint32_t ts_sec;
        uint32_t ts_usec;
        uint32_t ts_bus;
        log_get_next_frame_time (self->fp, &ts_sec, &ts_usec, &ts_bus );
        // 20 is the size of the timestamp field
        self->first_timestamp_offset = ftello (self->fp) - 20;
    } else {
        self->file_size = 0;;
        self->stream_id = NULL;
        self-> width = 0;
        self->height = 0;
        self->row_stride = 0;
        self->pixel_format = 0;
        self->frame_spacing = 0;
        self->first_timestamp_offset = 0;
    }

    return self;
}

void 
camlog_destroy( Camlog *self )
{
    if( self->fp ) {
        fclose( self->fp );
    }
    if( self->mode ) free(self->mode);
    if( self->stream_id ) free( self->stream_id );
    memset(self,0,sizeof(Camlog));
    free(self);
}

const char * 
camlog_get_stream_id( Camlog *self )
{
    return self->stream_id;
}

int 
camlog_seek_to_frame( Camlog *self, int frameno )
{
    uint64_t offset = self->first_timestamp_offset +
        (uint64_t) frameno * self->frame_spacing;

    fseeko (self->fp, offset, SEEK_SET);
    return 0;
}


int 
camlog_seek_to_offset( Camlog *self, int64_t offset, int *frameno )
{
    // TODO check that offset is aligned with the start of a data packet
    *frameno = (offset - self->first_timestamp_offset) / 
        self->frame_spacing;
    fseeko (self->fp, offset, SEEK_SET);
    return 0;
}

int 
camlog_get_format( Camlog *self, 
        int *width, int *height, int *row_stride, 
        uint32_t *pixel_format )
{
    *width = self->width;
    *height = self->height;
    *row_stride = self->row_stride;
    *pixel_format = self->pixel_format;
    return 0;
}

int 
camlog_get_next_frame( Camlog *self, uint8_t *buf, 
        int buf_size, uint64_t * source_uid )
{
    int status;
    uint16_t type = 0;
    uint32_t length;
    while (!log_get_field (&type, &length, self->fp)) {
        if (type == LOG_TYPE_FRAME_DATA) {
            break;
        }
        else if (source_uid && type == LOG_TYPE_SOURCE_UID) {
            log_get_uint64 (source_uid, self->fp);
            continue;
        }
        fseeko (self->fp, length, SEEK_CUR);
    }
    if (type != LOG_TYPE_FRAME_DATA ) {
        err("camlog: expected LOG_TYPE_FRAME_DATA, got %d\n", type);
        return -1;
    }
    if( length > buf_size) {
        err("camlog: read buffer is too small (%d bytes, need %d)\n",
                buf_size, length);
        return -1;
    }
    status = fread (buf, 1, length, self->fp);
    dbg("camlog: get_next_frame returned %d\n", status);
    if( length == status ) {
        return length;
    } else {
        err("camlog: error reading frame data\n");
        perror("fread");
        return -1;
    }
}

int 
camlog_get_next_frame_timestamp( Camlog *self, 
        int64_t *timestamp )
{
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t ts_bus;
    int status = 
        log_get_next_frame_time( self->fp, &ts_sec, &ts_usec, &ts_bus );
    if( 0 != status ) return -1;

    *timestamp = (uint64_t)ts_sec * 1000000 + ts_usec;
    return 0;
}

int 
camlog_count_frames( Camlog *self )
{
    return self->file_size / self->frame_spacing;
}

int 
camlog_write_stream_id( Camlog *self, const char *stream_id )
{
    if( self->mode[0] != 'w' ) return -1;
    int status;

    int len = strlen(stream_id);
    log_put_field (LOG_TYPE_STREAM_ID, len, self->fp);
    status = fwrite(stream_id, 1, len, self->fp);
    if( status > 0 ) {
        self->file_size += ftello( self->fp );
    }

    if( self->stream_id ) {
        err("WARNING:  already wrote a stream ID [%s]\n", self->stream_id );
        free( self->stream_id );
    }
    self->stream_id = strdup(stream_id);

    return status != len;
}

int 
camlog_write_frame( Camlog *self, int width, int height, int stride,
        int pixelformat, int64_t timestamp, int bus_timestamp,
        uint64_t source_uid,
        const uint8_t *data, int datalen, int64_t * file_offset )
{
    if( self->mode[0] != 'w' ) return -1;
    int status;

    if (file_offset)
        *file_offset = ftello (self->fp);

    log_put_field (LOG_TYPE_FRAME_FORMAT, 10, self->fp);
    log_put_short (width, self->fp);
    log_put_short (height, self->fp);
    log_put_short (stride, self->fp);
    log_put_long (pixelformat, self->fp);
    log_put_field (LOG_TYPE_FRAME_TIMESTAMP, 12, self->fp);
    int tv_sec = timestamp / 1000000;
    int tv_usec = timestamp % 1000000;
    dbg("camlog: tv_sec: %d tv_usec: %d datalen: %d\n", tv_sec, tv_usec, 
            datalen);
    log_put_long (tv_sec, self->fp);
    log_put_long (tv_usec, self->fp);
    log_put_long (bus_timestamp, self->fp);
    log_put_field (LOG_TYPE_SOURCE_UID, 8, self->fp);
    log_put_uint64 (source_uid, self->fp);
    log_put_field (LOG_TYPE_FRAME_DATA, datalen, self->fp);
    status = fwrite (data, 1, datalen, self->fp);
    self->file_size = ftello( self->fp );

    return status != datalen;
}

//static int
//log_seek_to_field_type( FILE *f, uint16_t type, uint32_t *length )
//{
//    uint16_t ftype;
//    uint32_t flen;
//    while( !log_get_field( &ftype, &flen, f )) {
//        if( ftype == type ) {
//            *length = flen;
//            return 0;
//        }
//        fseeko(f, flen, SEEK_CUR);
//    }
//    return -1;
//}

static int
log_get_frame_format (FILE * f, uint32_t * pf, uint16_t * width,
        uint16_t * height, uint16_t * stride, uint32_t * frame_spacing)
{
    uint16_t type = 0;
    uint32_t length;
    off_t fpos1 = 0, fpos2 = 0;

    while (!log_get_field (&type, &length, f)) {
        if (type == LOG_TYPE_FRAME_FORMAT) {
            fpos1 = ftello (f);
            break;
        }
        fseeko (f, length, SEEK_CUR);
    }
    if (type != LOG_TYPE_FRAME_FORMAT || length != 10)
        return -1;
    log_get_short (width, f);
    log_get_short (height, f);
    log_get_short (stride, f);
    log_get_long (pf, f);

    while (!log_get_field (&type, &length, f)) {
        if (type == LOG_TYPE_FRAME_FORMAT) {
            fpos2 = ftello (f);
            break;
        }
        fseeko (f, length, SEEK_CUR);
    }
    if (type != LOG_TYPE_FRAME_FORMAT || length != 10)
        return -1;

    *frame_spacing = (uint32_t) (fpos2 - fpos1);
    rewind (f);
    return 0;
}

static int
log_get_next_frame_time (FILE * f, uint32_t * sec, 
        uint32_t * usec, uint32_t * bus)
{
    uint16_t type = 0;
    uint32_t length;
    while (!log_get_field (&type, &length, f)) {
        if (type == LOG_TYPE_FRAME_TIMESTAMP)
            break;
        fseeko (f, length, SEEK_CUR);
    }
    if (type != LOG_TYPE_FRAME_TIMESTAMP || length != 12)
        return -1;
    log_get_long (sec, f);
    log_get_long (usec, f);
    log_get_long (bus, f);
    return 0;
}

static int 
log_get_stream_id (FILE *f, char **stream_id, int *len)
{
    rewind(f);
    uint16_t ftype;
    uint32_t flen;
    while( !log_get_field( &ftype, &flen, f )) {
        if( ftype == LOG_TYPE_STREAM_ID ) {
            *stream_id = (char*) calloc(1, flen+1);
            fread(*stream_id, 1, flen, f);
            *len = (int)flen;
            return 0;
        } else if( ftype == LOG_TYPE_FRAME_DATA ) {
            return -1;
        }
        fseeko(f, flen, SEEK_CUR);
    }
    return -1;
}
