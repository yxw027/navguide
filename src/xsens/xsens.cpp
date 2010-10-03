#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <common/serial.h>
#include <common/ioutils.h>
#include <common/timestamp.h>
#include <common/mathutil.h>

#include "xsens.h"

#define TIMEOUT 200

int xsens_read_packet(xsens_t *xs, uint8_t *buf);

xsens_t *xsens_create(const char *portname)
{  
    xsens_t *xs = (xsens_t*) calloc(sizeof(xsens_t), 1);

    xs->fd = serial_open(portname, 115200, 1);
    serial_set_N82 (xs->fd);
    xs->updatefrequency = 100; // Hz.

    if (xs->fd <= 0) {
        perror(portname);
        return NULL;
    }
    
    xsens_change_state(xs, 1);

    return xs;
}

void xsens_stop (xsens_t *xs) {
}

void xsens_destroy(xsens_t *xs)
{
    serial_close (xs->fd);
    xs->fd=-1;
    free(xs);
}

void
xsens_set_callback (xsens_t * xs, xsens_callback_func callback, void * user)
{
    xs->callback = callback;
    xs->user = user;
}

uint8_t xsens_compute_checksum(const uint8_t *buf)
{
    int len = buf[3];
    int i;
    uint8_t chk = 0;

    for (i = 1; i < 4+len; i++)
        chk += buf[i];

    return (-chk);
}

int xsens_transaction(xsens_t *xs, const uint8_t *request, uint8_t *reply)
{
    int res = 0;
    int msgid = request[2];

    int tries = 0;

resend:
    // write packet
    //fprintf (stderr, "Writing MID %x length %d\n", request[2], 5+request[3]);
    res = write_fully(xs->fd, request, 5+request[3]);
    if (res < 0) {
        fprintf (stderr, "Error: failed to write MID 0x%02x\n", request[2]);
        return -1;
    }

reread:
    tries++;
    if (tries > 20) {
        tries = 0;
        fprintf (stderr, "Warning: response not received, resending command...\n");
        goto resend;
    }

    // check for reply
    res = xsens_read_packet(xs, reply);
    if (res < 0) {
        fprintf (stderr, "Error waiting for MID %x\n", msgid+1);
        return -1;
    }
    if (res == 0) {// timeout
        fprintf (stderr, "Warning: timeout waiting for command response\n");
        goto resend;
    }
    if (reply[2]==msgid+1) // GOOD!
        return 0;
    if (reply[2]==66) {
        fprintf(stderr, "Warning: xsens reported transaction error\n");
    }

    goto reread;
}

// doesn't work?
int xsens_device_id(xsens_t *xs, uint8_t *id)
{
    (void) id;

    uint8_t request[256], reply[256];
    request[0] = 0xfa;
    request[1] = 0xff;
    request[2] = 0x00;
    request[3] = 0x00;
    request[4] = xsens_compute_checksum(request);

    xsens_transaction(xs, request, reply);

    printf("%02x%02x%02x%02x\n", reply[4], reply[5], reply[6], reply[7]);

    return 0; // success
}

int xsens_send_command (xsens_t * xs, uint8_t mid, uint8_t * data,
                        int len, uint8_t * reply)
{
    uint8_t request[5+len];

    request[0] = 0xfa;
    request[1] = 0xff;
    request[2] = mid;
    request[3] = len;
    memcpy (request + 4, data, len);
    request[4+len] = xsens_compute_checksum (request);

    return xsens_transaction (xs, request, reply);
}

int xsens_get_amd_setting (xsens_t * xs, uint16_t * val)
{
    uint8_t reply[256];

    if (xsens_send_command (xs, 0xA2, NULL, 0, reply) < 0)
        return -1;

    *val = (reply[4] << 8) | reply[5];
    return 0;
}

int xsens_set_amd_setting (xsens_t * xs, uint16_t val)
{
    uint8_t reply[256];
    uint8_t data[2];

    data[0] = val >> 8;
    data[1] = val & 0xff;

    if (xsens_send_command (xs, 0xA2, data, 2, reply) < 0)
        return -1;

    return 0;
}

static float bytes_to_float (uint8_t * b)
{
    union {
        float fv;
        uint32_t iv;
    } val;

    val.iv = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
    return val.fv;
}

static void float_to_bytes (uint8_t * b, float v)
{
    union {
        float fv;
        uint32_t iv;
    } val;

    val.fv = v;
    b[0] = (val.iv >> 24) & 0xff;
    b[1] = (val.iv >> 16) & 0xff;
    b[2] = (val.iv >> 8) & 0xff;
    b[3] = val.iv & 0xff;
}

int xsens_get_filter_settings (xsens_t * xs, float * gain,
                               float * magnetometer)
{
    uint8_t reply[256];
    uint8_t data;

    data = 0;
    if (xsens_send_command (xs, 0xA0, &data, 1, reply) < 0)
        return -1;

    *gain = bytes_to_float (reply + 5);

    data = 1;
    if (xsens_send_command (xs, 0xA0, &data, 1, reply) < 0)
        return -1;

    *magnetometer = bytes_to_float (reply + 5);
    
    return 0;
}

int xsens_set_filter_gain (xsens_t * xs, float gain)
{
    uint8_t reply[256];
    uint8_t data[6];

    data[0] = 0;
    float_to_bytes (data + 1, gain);
    data[5] = 0;
    if (xsens_send_command (xs, 0xA0, data, 6, reply) < 0)
        return -1;

    return 0;
}

int xsens_set_filter_magnetometer (xsens_t * xs, float weight)
{
    uint8_t reply[256];
    uint8_t data[6];

    data[0] = 1;
    float_to_bytes (data + 1, weight);
    data[5] = 0;
    if (xsens_send_command (xs, 0xA0, data, 6, reply) < 0)
        return -1;

    return 0;
}

int xsens_get_firmware_rev (xsens_t * xs, uint8_t * ver)
{
    uint8_t reply[256];

    if (xsens_send_command (xs, 0x12, NULL, 0, reply) < 0)
        return -1;

    ver[0] = reply[4];
    ver[1] = reply[5];
    ver[2] = reply[6];
    return 0;
}

int xsens_get_product_code (xsens_t * xs, char * str)
{
    uint8_t reply[256];

    if (xsens_send_command (xs, 0x1c, NULL, 0, reply) < 0)
        return -1;

    memcpy (str, reply + 4, reply[3]);
    str[reply[3]] = '\0';
    return 0;
}

int xsens_get_device_id (xsens_t * xs, uint32_t * id)
{
    uint8_t reply[256];

    if (xsens_send_command (xs, 0x00, NULL, 0, reply) < 0)
        return -1;

    *id = (reply[4] << 24) | (reply[5] << 16) | (reply[6] << 8) | reply[7];
    return 0;
}

void xsens_change_state(xsens_t *xs, int configmode)
{
    uint8_t request[256], reply[256];
    request[0] = 0xfa;
    request[1] = 0xff;
    request[2] = configmode ? XSENS_MID_GO_TO_CONFIG : XSENS_MID_GO_TO_MEASUREMENT;
    request[3] = 0;
    request[4] = xsens_compute_checksum(request);

    xsens_transaction(xs, request, reply);
}

void xsens_change_output_mode(xsens_t *xs, int mode, int settings)
{
    uint8_t request[256], reply[256];

    xsens_change_state(xs, 1);

    // set output mode
    request[0] = 0xfa;
    request[1] = 0xff;
    request[2] = XSENS_MID_SET_OUTPUT_MODE;
    request[3] = 2;
    request[4] = mode>>8;
    request[5] = mode&0xff;
    request[6] = xsens_compute_checksum(request);
    xsens_transaction(xs, request, reply);

    // set output setting
    request[0] = 0xfa;
    request[1] = 0xff;
    request[2] = XSENS_MID_SET_OUTPUT_SETTINGS;
    request[3] = 4;
    request[4] = 0;
    request[5] = 0;
    request[6] = 0;
    request[7] = settings&0xff;
    request[8] = xsens_compute_checksum(request);
    xsens_transaction(xs, request, reply);

    xsens_change_state(xs, 0);

    xs->mode = mode;
    xs->settings = settings;
}

int xsens_read_packet(xsens_t *xs, uint8_t *reply)
{
    int have = 0; // how many bytes current in our packet?
    int need;     // how many bytes do we need to read in our next read operation?
    uint8_t chk;
    int len;
    int res;
    int i;

readmore:
    // read header
    need = 4 - have;

    if (need > 0) {
        res = read_fully_timeout(xs->fd, &reply[have], need, TIMEOUT);
        if (res < 0) {
            fprintf (stderr, "Error waiting for new packet\n");
            goto error;
        }
#if 0
        printf ("read %d 1: ", xs->fd);
        for (i = 0; i < res; i++) {
            printf ("%02x ", reply[have+i]);
        }
        printf ("\n");
#endif
        if (res != need) {
            fprintf (stderr, "Timeout waiting for new packet (got %d bytes)\n",
                     have + res);
            return 0;
        }
        have += need;
    }

    // wait for start of packet byte and correct address
    if (reply[0] != 0xfa || reply[1] != 0xff) {
        fprintf (stderr, "Resyncing due to bad preamble %02x %02x\n",
                 reply[0], reply[1]);
        goto resync;
    }

    len = reply[3];
    //fprintf (stderr, "Got message MID %x length %d\n", reply[2], len + 5);

    if (len > 254) {
        fprintf (stderr, "Resyncing due to too long length\n");
        goto resync;
    }

    need = 4 + len + 1 - have;
    if (need > 0) {
        res = read_fully_timeout(xs->fd, &reply[have], need, TIMEOUT);
        if (res < 0) {
            fprintf (stderr, "Error waiting for message %x length %d\n",
                     reply[2], len);
            goto error;
        }
#if 0
        printf ("read %d 2: ", xs->fd);
        for (i = 0; i < res; i++) {
            printf ("%02x ", reply[have+i]);
        }
        printf ("\n");
#endif
        if (res != need) {
            fprintf (stderr, "Timeout waiting for message %x length %d (got %d bytes)\n",
                     reply[2], len, have + res);
            return 0;
        }
        have += need;
    }

    chk = reply[4+len];
    if (xsens_compute_checksum(reply) != chk) {
        //		printf("%02x %02x\n", xsens_compute_checksum(reply), chk);
        fprintf (stderr, "Resyncing due to bad checksum\n");
        goto resync;
    }

    //  good packet
    return have;

resync:
    // packet parsing failed. Look for the beginning of a new packet
    for (i = 1; i < have; i++) {
        if (reply[i] == 0xfa) { // new sync sequence?
            fprintf (stderr, "Resynced to %d bytes later\n", i);
            memmove(&reply[0], &reply[i], have-i);
            have-=i;
            goto readmore;
        }
    }

    have = 0;
    goto readmore;

error:
    return -2;
}

float get_float(const uint8_t *p, int offset)
{
    union {
        uint32_t iv;
        float fv;
    } intf;

    intf.iv = (p[offset]<<24) +
        (p[offset+1]<<16) +
        (p[offset+2]<<8) +
        p[offset+3];

    return intf.fv;
}

uint16_t get_uint16(const uint8_t *p, int offset)
{
    uint16_t i = (p[offset]<<8) + p[offset+1];

    return i;
}

static void
get_float_vector_as_doubles (const uint8_t * p, int offset, double * v, int num)
{
    int i;
    for (i = 0; i < num; i++) {
        v[i] = get_float (p, offset + 4 * i);
    }
}

void xsens_data_packet(xsens_t *xs, uint8_t *p)
{
    int64_t hosttime;
    xsens_data_t xd;

    int got_meas = 0;
    int got_quat = 0;
    int i = 4;

    hosttime = timestamp_now();

    if (xs->mode & XSENS_MODE_UNCALIBRATED_RAW) {
        fprintf (stderr, "TODO: Implement uncalibrated raw readout\n");
        i += 20;
    }

    if (xs->mode & XSENS_MODE_CALIBRATED) {
        // linear accelerations
        get_float_vector_as_doubles (p, i, xd.linear_accel, 3);
        // rotations
        get_float_vector_as_doubles (p, i+12, xd.rotation_rate, 3);
        // magnetometer
        get_float_vector_as_doubles (p, i+24, xd.magnetometer, 3);

        got_meas = 1;
        i += 36;
    }

    if (xs->mode & XSENS_MODE_ORIENTATION) {
        if ((xs->settings & XSENS_SETTING_ORIENTATION_MASK) ==
            XSENS_SETTING_EULER) {
            fprintf (stderr, "TODO: Implement euler readout\n");
            i += 12;
        }
        else if ((xs->settings & XSENS_SETTING_ORIENTATION_MASK) ==
                 XSENS_SETTING_QUATERNION) {
            get_float_vector_as_doubles (p, i, xd.quaternion, 4);
	    got_quat = 1;
            i += 16;
        }
        else if ((xs->settings & XSENS_SETTING_ORIENTATION_MASK) ==
                 XSENS_SETTING_MATRIX) {
            fprintf (stderr, "TODO: Implement matrix readout\n");
            i += 36;
        }
    }

    if (xs->settings & XSENS_SETTING_TIMESTAMPS) {
        int ticks = get_uint16(p, i);
        int dticks = ticks - xs->lastticks;
        double dt = 1000000.0 / xs->updatefrequency;
        int64_t devicetime;
        int64_t timeerr;

        int64_t hosttime = timestamp_now();

        //printf ("ticks %x\n", ticks);
        if (dticks < 0)
            dticks += 0x10000;
        xs->lastticks = ticks;
        xs->totalticks += dticks;

        // overestimate device time (that's where 1.01 comes from)
        devicetime = (int64_t)(xs->totalticks * (dt * 1.01));

        timeerr = (int64_t)hosttime - devicetime - xs->timeoffset;
        //printf ("%llu %llu %ld\n", hosttime, devicetime, timeerr);

        /* if timeerr is very large, resynchronize, emitting a warning. if
         * timer is negative, we're just adjusting our timebase (it means
         * we got a nice new low-latency measurement.) */
        if (timeerr > 1000000000LL || timeerr < 0) {
            xs->timeoffset = hosttime - devicetime;
        } 

        if (xs->callback) {
            if (got_meas & got_quat) {
                xs->callback (xs, devicetime + xs->timeoffset,
                        ticks, &xd, xs->user) ;
            }
        }
    }
}

// call this after you've set up your data to spool. it will handle call backs
// and stuff.
int xsens_run(xsens_t *xs)
{
    uint8_t reply[1024];

    while (1) {
        if (xsens_read_packet(xs, reply) < 0)
            return -1;

        if (reply[2]==XSENS_MID_MT_DATA) // data packet?
            xsens_data_packet(xs, reply);
    }
    return 0;
}

#if 0
int main(int argc, char *argv[])
{
    char *portname = argv[1];

    xsens_t *xs = xsens_create(portname);

    //  uint8_t devicestring[256];
    //  xsens_device_id(xs, devicestring);

    xsens_run(xs);
}
#endif
