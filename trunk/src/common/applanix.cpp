/* applanix.c:
 *
 * These functions decode the raw bytestream from the Applanix IMU and
 * convert it into C structs suitable for applications.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

#include <common/rotations.h>
#include <common/timestamp.h>
#include "applanix.h"



#define U16(d) *(uint16_t *)(d)
#define S16(d) *(int16_t *)(d)
#define S32(d) *(int32_t *)(d)
#define U32(d) *(uint32_t *)(d)
#define F32(d) *(float *)(d)
#define F64(d) *(double *)(d)

#define ENCODE_U16(d,v) do { \
    *(d) = (v) & 0xff; \
    *(d+1) = ((v) >> 8) & 0xff; \
} while (0)

#define ENCODE_F32(d,v) *(float *)(d) = (v)

int
applanix_decode (applanix_data_t * out, const uint8_t * data, int len)
{
    if (len < 8)
        return -1;

    int payload_len = applanix_payload_length (data);
    if (len < payload_len + 8)
        return -1;

    if (applanix_compute_checksum (data) != applanix_checksum (data)) {
        fprintf (stderr, "Warning: bad applanix checksum\n");
        return -1;
    }

    if (!memcmp (data, "$GRP", 4))
        out->type = APPLANIX_DATA_GRP;
    else if (!memcmp (data, "$MSG", 4))
        out->type = APPLANIX_DATA_MSG;
    else
        return -1;

    out->id = applanix_groupid (data);

    if (out->type == APPLANIX_DATA_GRP) {
        switch (out->id) {
            case 1:
                applanix_decode_grp1 (&out->grp1, data);
                break;
            case 2:
                applanix_decode_grp2 (&out->grp2, data);
                break;
            case 3:
            case 11:
                applanix_decode_grp3 (&out->grp3, data);
                break;
            case 4:
                applanix_decode_grp4 (&out->grp4, data);
                break;
            case 10:
                applanix_decode_grp10 (&out->grp10, data);
                break;
            case 14:
                applanix_decode_grp14 (&out->grp14, data);
                break;
            case 15:
                applanix_decode_grp15 (&out->grp15, data);
                break;
            case 10002:
                applanix_decode_grp10002 (&out->grp10002, data);
                break;
            case 10006:
                applanix_decode_grp10006 (&out->grp10006, data);
                break;
        }
    }
    else if (out->type == APPLANIX_DATA_MSG) {
        switch (out->id) {
            case 0:
                applanix_decode_msg0 (&out->msg0, data);
                break;
            case 20:
                applanix_decode_msg20 (&out->msg20, data);
                break;
            case 22:
                applanix_decode_msg22 (&out->msg22, data);
                break;
            case 50:
                applanix_decode_msg50 (&out->msg50, data);
                break;
            case 51:
            case 52:
            case 61:
                applanix_decode_msg52 (&out->msg52, data);
                break;
        }
    }
    return payload_len + 8;
}

void
applanix_print (FILE * f, const applanix_data_t * a)
{
    if (a->type == APPLANIX_DATA_GRP) {
        if (a->id == 1)
            applanix_print_grp1 (f, &a->grp1);
        else if (a->id == 2)
            applanix_print_grp2 (f, &a->grp2);
        else if (a->id == 3 || a->id == 11)
            applanix_print_grp3 (f, &a->grp3);
        else if (a->id == 4)
            applanix_print_grp4 (f, &a->grp4);
        else if (a->id == 10)
            applanix_print_grp10 (f, &a->grp10);
        else if (a->id == 14)
            applanix_print_grp14 (f, &a->grp14);
        else if (a->id == 15)
            applanix_print_grp15 (f, &a->grp15);
        else if (a->id == 10002)
            applanix_print_grp10002 (f, &a->grp10002);
        else if (a->id == 10006)
            applanix_print_grp10006 (f, &a->grp10006);
    }
    else if (a->type == APPLANIX_DATA_MSG) {
        if (a->id == 0)
            applanix_print_msg0 (f, &a->msg0);
        else if (a->id == 20)
            applanix_print_msg20 (f, &a->msg20);
        else if (a->id == 22)
            applanix_print_msg22 (f, &a->msg22);
        else if (a->id == 50)
            applanix_print_msg50 (f, &a->msg50);
        else if (a->id == 51 || a->id == 52 || a->id == 61)
            applanix_print_msg52 (f, &a->msg52);
    }
}

int64_t
applanix_get_pos_time (const uint8_t * data)
{
    /* Only GRP messages have a timestamp */
    if (memcmp (data, "$GRP", 4))
        return -1;

    /* See page 5 of Applanix interface docs */
    if ((data[32] & 0x0f) == 0)
        /* POS time is in time field 1 */
        return (int64_t)(F64(data + 8) * 1000000);
    else if ((data[32] & 0xf0) == 0)
        /* POS time is in time field 2 */
        return (int64_t)(F64(data + 16) * 1000000);

    return -1;
}

int
applanix_decode_timedist (applanix_timedist_t *grp, const uint8_t * data)
{
    grp->time1 = F64(&data[0]);
    grp->time2 = F64(&data[8]);
    grp->dist_tag = F64(&data[16]);
    grp->time_type = data[24];
    grp->dist_type = data[25];
    return 0;
}

static const char * time_types[] = { "POS", "GPS", "UTC", "User" };
static const char * dist_types[] = { "N/A", "POS", "DMI" };

void
applanix_print_timedist (FILE * f, const applanix_timedist_t * tidi)
{
    fprintf (f, "  %s time: %.3lf,",
            time_types[tidi->time_type & 0xf], tidi->time1);
    fprintf (f, " %s time: %.3lf,",
            time_types[(tidi->time_type & 0xf0) >> 4], tidi->time2);
    fprintf (f, " %s dist: %.3lf\n",
            dist_types[tidi->dist_type], tidi->dist_tag);
}

int 
applanix_decode_grp1(applanix_grp1_t *grp, const uint8_t *data)
{
    applanix_decode_timedist(&grp->tidi, &data[4+4]);
    grp->lat = F64(&data[4+30]);
    grp->lon = F64(&data[4+38]);
    grp->alt = F64(&data[4+46]);

    // This offset is now in position read from configfile
	// NAD83 -> IRTF00 hack for site visit
    // Waymouth offset
    //grp->lat += 12.1944e-6;
    //grp->lon += 2.6528e-6;
    //grp->alt -= 1.2280;

    grp->vn = F32(&data[4+54]);
    grp->ve = F32(&data[4+58]);
    grp->vd = F32(&data[4+62]);
    grp->roll = F64(&data[4+66]);
    grp->pitch = F64(&data[4+74]);
    grp->heading = F64(&data[4+82]);
    grp->wander = F64(&data[4+90]);
    grp->track = F32(&data[4+98]);
    grp->speed = F32(&data[4+102]);
    grp->rate_lon = F32(&data[4+106]);
    grp->rate_trans = F32(&data[4+110]);
    grp->rate_down = F32(&data[4+114]);
    grp->acc_lon = F32(&data[4+118]);
    grp->acc_trans = F32(&data[4+122]);
    grp->acc_down = F32(&data[4+126]);
    grp->status = data[4+130];
    // successful decode.
    return 0;
}

static const char * alignment_str[] = {
    "Full navigation",
    "Fine alignment is active (head. err < 15 deg)",
    "GC CHI 2 (w/ GPS, head. err > 15 deg)",
    "PC CHI 2 (w/o GPS, head. err > 15 deg)",
    "GC CHI 1 (w/ GPS, head. err > 45 deg)",
    "PC CHI 1 (w/o GPS, head. err > 45 deg)",
    "Coarse leveling is active",
    "Initial solution assigned",
    "No valid solution",
};

const char *
applanix_nav_status_str (int status)
{
    return alignment_str[status];
}

void
applanix_print_grp1 (FILE * f, const applanix_grp1_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);
    fprintf (f, "  Lat: %13.8f  Lon: %13.8f  Alt: %11.3f\n",
            grp->lat, grp->lon, grp->alt);
    fprintf (f, "  Vel N: %9.2f  E: %10.2f  D: %10.2f\n",
            grp->vn, grp->ve, grp->vd);
    fprintf (f, "  Roll: %10.2f  Pitch: %6.2f  Head: %7.2f\n",
            grp->roll, grp->pitch, grp->heading);
    fprintf (f, "  Wander: %8.2f  Track: %6.2f  Speed: %6.2f\n",
            grp->wander, grp->track, grp->speed);
    fprintf (f, "  Rates L: %7.2f  T: %10.2f  D: %10.2f\n",
            grp->rate_lon, grp->rate_trans, grp->rate_down);
    fprintf (f, "  Accel L: %7.2f  T: %10.2f  D: %10.2f\n",
            grp->acc_lon, grp->acc_trans, grp->acc_down);
    fprintf (f, "  Status: %d  %s\n", grp->status, alignment_str[grp->status]);
    fprintf (f, "\n");
}


int
applanix_decode_grp2 (applanix_grp2_t * grp, const uint8_t * d)
{
    applanix_decode_timedist (&grp->tidi, d + 4 + 4);
    grp->np_err = F32 (d + 4 + 30);
    grp->ep_err = F32 (d + 4 + 34);
    grp->dp_err = F32 (d + 4 + 38);
    grp->nv_err = F32 (d + 4 + 42);
    grp->ev_err = F32 (d + 4 + 46);
    grp->dv_err = F32 (d + 4 + 50);
    grp->roll_err = F32 (d + 4 + 54);
    grp->pitch_err = F32 (d + 4 + 58);
    grp->heading_err = F32 (d + 4 + 62);
    grp->ellipse_maj = F32 (d + 4 + 66);
    grp->ellipse_min = F32 (d + 4 + 70);
    grp->ellipse_ang = F32 (d + 4 + 74);
    return 0;
}


void
applanix_print_grp2 (FILE * f, const applanix_grp2_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);
    fprintf (f, "  Pos RMS  N: %7.2f  E: %7.2f  D: %7.2f\n",
            grp->np_err, grp->ep_err, grp->dp_err);
    fprintf (f, "  Vel RMS  N: %7.2f  E: %7.2f  D: %7.2f\n",
            grp->nv_err, grp->ev_err, grp->dv_err);
    fprintf (f, "  Att RMS  R: %7.2f  P: %7.2f  H: %7.2f\n",
            grp->roll_err, grp->pitch_err, grp->heading_err);
    fprintf (f, "  Ellipse Maj: %6.2f  Min: %6.2f  Ang: %6.2f\n",
            grp->ellipse_maj, grp->ellipse_min, grp->ellipse_ang);
}

int
applanix_decode_grp3 (applanix_grp3_t * grp, const uint8_t * d)
{
    applanix_decode_timedist (&grp->tidi, d + 4 + 4);
    grp->solution_status = d[34];
    grp->num_sv = d[35];
    grp->status_bytes = U16 (d + 36);
    int i;
    for (i = 0; i < grp->status_bytes / 20; i++) {
        const uint8_t * o = d + 38 + i * 20;
        grp->channel[i].prn = U16 (o);
        grp->channel[i].status = U16 (o + 2);
        grp->channel[i].azimuth = F32 (o + 4);
        grp->channel[i].elevation = F32 (o + 8);
        grp->channel[i].L1_snr = F32 (o + 12);
        grp->channel[i].L2_snr = F32 (o + 16);
    }
    grp->hdop = F32 (d + 38 + grp->status_bytes);
    grp->vdop = F32 (d + 38 + grp->status_bytes + 4);
    grp->dgps_latency = F32 (d + 38 + grp->status_bytes + 8);
    grp->dgps_reference_id = U16 (d + 38 + grp->status_bytes + 12);
    grp->gps_week = U32 (d + 38 + grp->status_bytes + 14);
    grp->gps_type = U16 (d + 38 + grp->status_bytes + 34);
    grp->gps_status = U32 (d + 38 + grp->status_bytes + 36);
    return 0;
}

void
applanix_print_grp3 (FILE * f, const applanix_grp3_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);
    fprintf (f, "  Solution status: %d,", grp->solution_status);
    fprintf (f, "  Num SV: %d, Status bytes: %d\n", grp->num_sv,
            grp->status_bytes);
    int i;
    for (i = 0; i < grp->status_bytes / 20; i++) {
        fprintf (f, "   %2d Status: %d, Az %5.1f El %4.1f  L1 SNR %5.2f  L2 SNR %5.2f\n",
                grp->channel[i].prn, grp->channel[i].status,
                grp->channel[i].azimuth, grp->channel[i].elevation,
                grp->channel[i].L1_snr, grp->channel[i].L2_snr);
    }
    fprintf (f, "  HDOP: %f, VDOP: %f\n", grp->hdop, grp->vdop);
    fprintf (f, "  DGPS Latency: %f, Reference ID: %d\n", grp->dgps_latency,
            grp->dgps_reference_id);
    fprintf (f, "  GPS Type: %d, GPS Status: 0x%x, GPS Week: %u\n",
            grp->gps_type, grp->gps_status, grp->gps_week);
    fprintf (f, "\n");
}

int 
applanix_decode_grp4(applanix_grp4_t *grp, const uint8_t * data)
{
    applanix_decode_timedist(&grp->tidi, &data[4+4]);
    grp->dx = S32(&data[4+30]);
    grp->dy = S32(&data[4+34]);
    grp->dz = S32(&data[4+38]);
    grp->dthx = S32(&data[4+42]);
    grp->dthy = S32(&data[4+46]);
    grp->dthz = S32(&data[4+50]);
    grp->data_status = data[4+54];
    grp->imu_type = data[4+55];
    grp->imu_rate = data[4+56];
    grp->imu_status = U16(data+4+57);
    // successful decode.
    return 0;
}

void
applanix_print_grp4 (FILE * f, const applanix_grp4_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);
    fprintf (f, "  Dv:  x: %9d  y: %9d  z: %9d\n",
            grp->dx, grp->dy, grp->dz);
    fprintf (f, "  Dth: x: %9d  y: %9d  z: %9d\n",
            grp->dthx, grp->dthy, grp->dthz);
    fprintf (f, "  Data Status: 0x%x,", grp->data_status);
    fprintf (f, " IMU Type: %d,", grp->imu_type);
    fprintf (f, " IMU Status: 0x%x\n", grp->imu_status);
    fprintf (f, "\n");
}

int 
applanix_decode_grp10 (applanix_grp10_t *grp, const uint8_t * data)
{
    applanix_decode_timedist(&grp->tidi, &data[4+4]);

    grp->status_A = U32 (data + 34);
    grp->status_B = U32 (data + 38);
    grp->status_C = U32 (data + 42);
    grp->FDIR1 = U32 (data + 46);
    grp->FDIR1_imu = U16 (data + 50);
    grp->FDIR2 = U16 (data + 52);
    grp->FDIR3 = U16 (data + 54);
    grp->FDIR4 = U16 (data + 56);
    grp->FDIR5 = U16 (data + 58);
    // successful decode.
    return 0;
}

static const char * status_A_desc[] = {
    "Coarse leveling active",
    "Coarse leveling failed",
    "Quadrant resolved",
    "Fine align active",
    "Inertial navigator initialized",
    "Inertial navigator alignment active",
    "Degraded navigation solution",
    "Full navigation solution",
    "Initial position valid",
    "Reference to Primary GPS Lever arms = 0",
    "Reference to Sensor 1 Lever arms = 0",
    "Reference to Sensor 2 Lever arms = 0",
    "Logging Port file write error",
    "Logging Port file open",
    "Logging Port logging enabled",
    "Logging Port device full",
    "RAM configuration differs from NVM",
    "NVM write successful",
    "NVM write fail",
    "NVM read fail",
    "CPU loading exceeds 55% threshold",
    "CPU loading exceeds 85% threshold",
};

static const char * status_B_desc[] = {
    "User attitude RMS performance",
    "User heading RMS performance",
    "User position RMS performance",
    "User velocity RMS performance",
    "GAMS calibration in progress",
    "GAMS calibration complete",
    "GAMS calibration failed",
    "GAMS calibration requested",
    "GAMS installation parameters valid",
    "GAMS solution in use",
    "GAMS solution OK",
    "GAMS calibration suspended",
    "GAMS calibration forced",
    "Primary GPS navigation solution in use",
    "Primary GPS initialization failed",
    "Primary GPS reset command sent",
    "Primary GPS configuration file sent",
    "Primary GPS not configured",
    "Primary GPS in C/A mode",
    "Primary GPS in Differential mode",
    "Primary GPS in float RTK mode",
    "Primary GPS in wide lane RTK mode",
    "Primary GPS in narrow lane RTK mode",
    "Primary GPS observables in use",
    "Secondary GPS observables in use",
    "Auxiliary GPS navigation solution in use",
    "Auxiliary GPS in P-code mode",
    "Auxiliary GPS in Differential mode",
    "Auxiliary GPS in float RTK mode",
    "Auxiliary GPS in wide lane RTK mode",
    "Auxiliary GPS in narrow lane RTK mode",
    "Primary GPS in P-code mode",
};

static const char * status_C_desc[] = {
    "Reserved",
    "Reserved",
    "DMI data in use",
    "ZUPD processing enabled",
    "ZUPD in use",
    "Position fix in use",
    "RTCM differential corrections in use",
    "RTCM RTK messages in use",
    "Reserved",
    "CMR RTK messages in use",
    "IIN in DR mode",
    "IIN GPS aiding is loosely coupled",
    "IIN in C/A GPS aided mode",
    "IIN in RTCM DGPS aided mode",
    "IIN in code DGPS aided mode",
    "IIN in float RTK aided mode",
    "IIN in wide lane RTK aided mode",
    "IIN in narrow lane RTK aided mode",
    "Received RTCM Type 1 message",
    "Received RTCM Type 3 message",
    "Received RTCM Type 9 message",
    "Received RTCM Type 18 message",
    "Received RTCM Type 19 message",
    "Received CMR Type 0 message",
    "Received CMR Type 1 message",
    "Received CMR Type 2 message",
    "Received CMR Type 94 message",
    "Reserved",
};

static const char * FDIR1_desc[] = {
    "IMU-POS checksum error",
    "IMU status bit set by IMU",
    "Successive IMU failures",
    "IIN configuration mismatch failure",
    NULL,
    "Primary GPS not in Navigation mode",
    "Primary GPS not available for alignment",
    "Primary GPS data gap",
    "Primary GPS PPS time gap",
    "Primary GPS time recovery data not received",
    "Primary GPS observable data gap",
    "Primary GPS ephermeris data gap",
    "Primary GPS excessive lock-time resets",
    "Primary GPS missing ephermeris",
    NULL,
    NULL,
    "Primary GPS SNR failure",
    "Base GPS data gap",
    "Base GPS parity error",
    "Base GPS message rejected",
    "Secondary GPS data gap",
    "Secondary GPS observable data gap",
    "Secondary GPS SNR failure",
    "Secondary GPS excessive lock-time resets",
    NULL,
    "Auxiliary GPS data gap",
    "GAMS ambiguity resolution failed",
    "Reserved",
    NULL,
    NULL,
    "IIN WL ambiguity error",
    "IIN NL ambiguity error",
};

static const char * FDIR2_desc[] = {
    "Inertial speed exceeds maximum",
    "Primary GPS velocity exceeds maximum",
    "Primary GPS position error exceeds maximum",
    "Auxiliary GPS position error exceeds maximum",
    "DMI speed exceeds maximum",
};

static const char * FDIR4_desc[] = {
    "Primary GPS position rejected",
    "Primary GPS velocity rejected",
    "GAMS heading rejected",
    "Auxiliary GPS data rejected",
    "DMI data rejected",
    "Primary GPS observables rejected",
};

static const char * FDIR5_desc[] = {
    "X accelerometer failure",
    "Y accelerometer failure",
    "Z accelerometer failure",
    "X gyro failure",
    "Y gyro failure",
    "Z gyro failure",
    "Excessive GAMS heading offset",
    "Excessive primary GPS lever arm error",
    "Excessive auxiliary 1 GPS lever arm error",
    "Excessive auxiliary 2 GPS lever arm error",
    "Excessive POS position error RMS",
    "Excessive primary GPS clock drift",
};

static void
print_bitfield_desc (FILE * f, uint32_t v, const char * desc[], int len)
{
    int i;
    for (i = 0; i < len; i++)
        if (((v >> i) & 1) && desc[i])
            fprintf (f, "    %s\n", desc[i]);
}

void
applanix_print_grp10 (FILE * f, const applanix_grp10_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);

    if (grp->status_A) {
        fprintf (f, "  Status A (0x%x):\n", grp->status_A);
        print_bitfield_desc (f, grp->status_A, status_A_desc, 22);
    }
    if (grp->status_B) {
        fprintf (f, "  Status B (0x%x):\n", grp->status_B);
        print_bitfield_desc (f, grp->status_B, status_B_desc, 32);
    }
    if (grp->status_C) {
        fprintf (f, "  Status C (0x%x):\n", grp->status_C);
        print_bitfield_desc (f, grp->status_C, status_C_desc, 28);
    }
    if (grp->FDIR1) {
        fprintf (f, "  Faults 1 (0x%x):\n", grp->FDIR1);
        print_bitfield_desc (f, grp->FDIR1, FDIR1_desc, 32);
    }
    fprintf (f, "  Number of IMU status failures: %d\n", grp->FDIR1_imu);
    if (grp->FDIR2) {
        fprintf (f, "  Faults 2 (0x%x):\n", grp->FDIR2);
        print_bitfield_desc (f, grp->FDIR2, FDIR2_desc, 5);
    }
    if (grp->FDIR3) {
        fprintf (f, "  Faults 3 (0x%x)\n", grp->FDIR3);
    }
    if (grp->FDIR4) {
        fprintf (f, "  Faults 4 (0x%x):\n", grp->FDIR4);
        print_bitfield_desc (f, grp->FDIR4, FDIR4_desc, 6);
    }
    if (grp->FDIR5) {
        fprintf (f, "  Faults 5 (0x%x):\n", grp->FDIR5);
        print_bitfield_desc (f, grp->FDIR5, FDIR5_desc, 12);
    }
    fprintf (f, "\n");
}

int
applanix_decode_grp14 (applanix_grp14_t * grp, const uint8_t * data)
{
    applanix_decode_timedist(&grp->tidi, &data[4+4]);

    grp->status = U16 (data + 34);

    grp->primary_GPS[0] = F32 (data + 36);
    grp->primary_GPS[1] = F32 (data + 40);
    grp->primary_GPS[2] = F32 (data + 44);
    grp->primary_GPS_FOM = U16 (data + 48);

    grp->aux_GPS1[0] = F32 (data + 50);
    grp->aux_GPS1[1] = F32 (data + 54);
    grp->aux_GPS1[2] = F32 (data + 58);
    grp->aux_GPS1_FOM = U16 (data + 62);

    grp->aux_GPS2[0] = F32 (data + 64);
    grp->aux_GPS2[1] = F32 (data + 68);
    grp->aux_GPS2[2] = F32 (data + 72);
    grp->aux_GPS2_FOM = U16 (data + 76);

    grp->DMI_lever[0] = F32 (data + 78);
    grp->DMI_lever[1] = F32 (data + 82);
    grp->DMI_lever[2] = F32 (data + 86);
    grp->DMI_lever_FOM = U16 (data + 90);

    grp->DMI_scale = F32 (data + 92);
    grp->DMI_scale_FOM = U16 (data + 96);
    return 0;
}

static const char * calib_status_desc[] = {
    "Ref to Primary GPS lever arm calibration in progress",
    "Ref to Aux 1 GPS lever arm calibration in progress",
    "Ref to Aux 2 GPS lever arm calibration in progress",
    "Ref to DMI lever arm calibration in progress",
    "DMI scale factor calibration in progress",
    "Reserved",
    "Ref to Position Fix lever arm calibration in progress",
    "Reserved",
    "Ref to Primary GPS lever arm calibration complete",
    "Ref to Aux 1 GPS lever arm calibration complete",
    "Ref to Aux 2 GPS lever arm calibration complete",
    "Ref to DMI lever arm calibration complete",
    "DMI scale factor calibration complete",
    "Reserved",
    "Ref to Position Fix lever arm calibration complete",
    "Reserved",
};

void
applanix_print_grp14 (FILE * f, const applanix_grp14_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);

    fprintf (f, "  Status: %04x\n", grp->status);
    int i;
    for (i = 0; i < 16; i++) {
        if ((grp->status >> i) & 0x1)
            fprintf (f, "    %s\n", calib_status_desc[i]);
    }
    fprintf (f, "  Ref to GPS          X: %6.3f  Y: %6.3f  Z: %6.3f  FOM: %d\n",
            grp->primary_GPS[0], grp->primary_GPS[1], grp->primary_GPS[2],
            grp->primary_GPS_FOM);
    fprintf (f, "  Ref to Aux 1 GPS    X: %6.3f  Y: %6.3f  Z: %6.3f  FOM: %d\n",
            grp->aux_GPS1[0], grp->aux_GPS1[1], grp->aux_GPS1[2],
            grp->aux_GPS1_FOM);
    fprintf (f, "  Ref to Aux 2 GPS    X: %6.3f  Y: %6.3f  Z: %6.3f  FOM: %d\n",
            grp->aux_GPS2[0], grp->aux_GPS2[1], grp->aux_GPS2[2],
            grp->aux_GPS2_FOM);
    fprintf (f, "  Ref to DMI          X: %6.3f  Y: %6.3f  Z: %6.3f  FOM: %d\n",
            grp->DMI_lever[0], grp->DMI_lever[1], grp->DMI_lever[2],
            grp->DMI_lever_FOM);
    fprintf (f, "  DMI scale:  %7.2f   FOM: %d\n",
            grp->DMI_scale, grp->DMI_scale_FOM);
    fprintf (f, "\n");
}

int
applanix_decode_grp15 (applanix_grp15_t * grp, const uint8_t * data)
{
    applanix_decode_timedist(&grp->tidi, &data[4+4]);

    grp->dist_signed = F64 (data + 34);
    grp->dist_unsigned = F64 (data + 42);
    grp->scale = U16 (data + 50);
    grp->status = data[52];
    grp->type = data[53];
    grp->rate = data[54];
    return 0;
}

static const char * dmi_types[] = {
    "None",
    "Pulse and Dir",
    "Quadrature",
};

static const char * dmi_rates[] = {
    "50 Hz",
    "100 Hz",
    "200 Hz",
    "400 Hz",
    "125 Hz",
};

void
applanix_print_grp15 (FILE * f, const applanix_grp15_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);

    fprintf (f, "  Signed dist: %.3f  Unsigned dist: %.3f  Scale: %d\n",
            grp->dist_signed, grp->dist_unsigned, grp->scale);
    fprintf (f, "  Status: %d  Type: %d (%s)  Rate: %d (%s)\n",
            grp->status, grp->type, dmi_types[grp->type],
            grp->rate, grp->rate < 5 ? dmi_rates[grp->rate] : "N/A");
    fprintf (f, "\n");
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

int
applanix_decode_grp10002 (applanix_grp10002_t * grp, const uint8_t * data)
{
    applanix_decode_timedist (&grp->tidi, &data[4+4]);

    memcpy (grp->header, data + 34, 6);
    grp->header[6] = '\0';

    grp->length = U16 (data + 40);
    memcpy (grp->data, data + 42, MIN (grp->length, 64));
    grp->checksum = U16 (data + 42 + grp->length);
    return 0;
}

void
applanix_print_grp10002 (FILE * f, const applanix_grp10002_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);
    fprintf (f, "  IMU Header: %.6s\n", grp->header);
    fprintf (f, "  Data length: %d\n", grp->length);
    fprintf (f, " ");
    for (int i = 0; i < MIN (grp->length, 64); i++)
        fprintf (f, " %02x", grp->data[i]);
    fprintf (f, "\n\n");
}

int
applanix_decode_grp10006 (applanix_grp10006_t * grp, const uint8_t * data)
{
    applanix_decode_timedist(&grp->tidi, &data[4+4]);

    grp->count_signed = S32 (data + 34);
    grp->count_unsigned = U32 (data + 38);
    grp->event_count = S32 (data + 42);
    grp->reserved_count = U32 (data + 46);
    return 0;
}

void
applanix_print_grp10006 (FILE * f, const applanix_grp10006_t * grp)
{
    applanix_print_timedist (f, &grp->tidi);

    fprintf (f, "  Signed count: %11d    Unsigned count: %10d\n",
            grp->count_signed, grp->count_unsigned);
    fprintf (f, "  Event count:  %11d    Reserved count: %10d\n",
            grp->event_count, grp->reserved_count);
    fprintf (f, "\n");
}

int
applanix_decode_msg0 (applanix_msg0_t *msg, const uint8_t * data)
{
    msg->transaction = U16 (data + 8);
    msg->id = U16 (data + 10);
    msg->response_code = U16 (data + 12);
    msg->parameters_status = data[14];
    memcpy (msg->parameter_name, data + 15, 32);
    return 0;
}

static const char * response_desc[] = {
    "Not applicable",
    "Message accepted",
    "Message accepted -- too long",
    "Message accepted -- too short",
    "Message parameter error",
    "Not applicable in current state",
    "Data not available",
    "Message start error",
    "Message end error",
    "Byte count error",
    "Checksum error",
};

void
applanix_print_msg0 (FILE * f, const applanix_msg0_t * msg)
{
    fprintf (f, "  Transaction: %d  Msg ID: %d\n", msg->transaction, msg->id);
    fprintf (f, "  Response Code: %d (%s)\n", msg->response_code,
            response_desc[msg->response_code]);
    fprintf (f, "  %s\n", msg->parameters_status ? "Some parameters changed" :
            "No change in parameters");
    if (strlen ((char *) msg->parameter_name))
        fprintf (f, "  Error in parameter: %s\n", msg->parameter_name);
    fprintf (f, "\n");
}

int
applanix_decode_msg20 (applanix_msg20_t *msg, const uint8_t * data)
{
    msg->transaction = U16 (data + 8);
    msg->time_types = data[10];
    msg->dist_type = data[11];
    msg->autostart = data[12];
    msg->ref_to_IMU[0] = F32 (data + 13);
    msg->ref_to_IMU[1] = F32 (data + 17);
    msg->ref_to_IMU[2] = F32 (data + 21);
    msg->ref_to_GPS[0] = F32 (data + 25);
    msg->ref_to_GPS[1] = F32 (data + 29);
    msg->ref_to_GPS[2] = F32 (data + 33);
    msg->ref_to_aux1_GPS[0] = F32 (data + 37);
    msg->ref_to_aux1_GPS[1] = F32 (data + 41);
    msg->ref_to_aux1_GPS[2] = F32 (data + 45);
    msg->ref_to_aux2_GPS[0] = F32 (data + 49);
    msg->ref_to_aux2_GPS[1] = F32 (data + 53);
    msg->ref_to_aux2_GPS[2] = F32 (data + 57);
    msg->IMU_wrt_ref_angles[0] = F32 (data + 61);
    msg->IMU_wrt_ref_angles[1] = F32 (data + 65);
    msg->IMU_wrt_ref_angles[2] = F32 (data + 69);
    msg->ref_wrt_vehicle_angles[0] = F32 (data + 73);
    msg->ref_wrt_vehicle_angles[1] = F32 (data + 77);
    msg->ref_wrt_vehicle_angles[2] = F32 (data + 81);
    msg->multipath = data[85];
    return 0;
}

static const char * multipath_types[] = {
    "Low",
    "Medium",
    "High",
};

void
applanix_print_msg20 (FILE * f, const applanix_msg20_t * msg)
{
    fprintf (f, "  Transaction: %d\n", msg->transaction);
    fprintf (f, "  Time 1 type: %d (%s)  Time 2 type: %d (%s)  Dist type: %d (%s)\n",
            msg->time_types & 0xf, time_types[msg->time_types & 0xf],
            (msg->time_types & 0xf0) >> 4,
            time_types[(msg->time_types & 0xf0) >> 4],
            msg->dist_type, dist_types[msg->dist_type]);
    fprintf (f, "  Autostart: %s     Multipath: %d (%s)\n",
            msg->autostart ? "Enabled " : "Disabled",
            msg->multipath, multipath_types[msg->multipath]);
    fprintf (f, "  Ref to IMU          X: %6.3f  Y: %6.3f  Z: %6.3f\n",
            msg->ref_to_IMU[0], msg->ref_to_IMU[1], msg->ref_to_IMU[2]);
    fprintf (f, "  Ref to GPS          X: %6.3f  Y: %6.3f  Z: %6.3f\n",
            msg->ref_to_GPS[0], msg->ref_to_GPS[1], msg->ref_to_GPS[2]);
    fprintf (f, "  Ref to Aux 1 GPS    X: %6.3f  Y: %6.3f  Z: %6.3f\n",
            msg->ref_to_aux1_GPS[0], msg->ref_to_aux1_GPS[1],
            msg->ref_to_aux1_GPS[2]);
    fprintf (f, "  Ref to Aux 2 GPS    X: %6.3f  Y: %6.3f  Z: %6.3f\n",
            msg->ref_to_aux2_GPS[0], msg->ref_to_aux2_GPS[1],
            msg->ref_to_aux2_GPS[2]);
    fprintf (f, "  IMU wrt Ref angles  X: %6.2f  Y: %6.2f  Z: %6.2f\n",
            msg->IMU_wrt_ref_angles[0], msg->IMU_wrt_ref_angles[1],
            msg->IMU_wrt_ref_angles[1]);
    fprintf (f, "  Ref wrt Veh angles  X: %6.2f  Y: %6.2f  Z: %6.2f\n",
            msg->ref_wrt_vehicle_angles[0], msg->ref_wrt_vehicle_angles[1],
            msg->ref_wrt_vehicle_angles[1]);
    fprintf (f, "\n");
}

int
applanix_decode_msg22 (applanix_msg22_t *msg, const uint8_t * data)
{
    msg->transaction = U16 (data + 8);
    msg->DMI_scale = F32 (data + 10);
    msg->ref_to_DMI[0] = F32 (data + 14);
    msg->ref_to_DMI[1] = F32 (data + 18);
    msg->ref_to_DMI[2] = F32 (data + 22);
    msg->reserved[0] = F32 (data + 26);
    msg->reserved[1] = F32 (data + 30);
    msg->reserved[2] = F32 (data + 34);
    msg->reserved[3] = F32 (data + 38);
    msg->reserved[4] = F32 (data + 42);
    msg->reserved[5] = F32 (data + 46);
    msg->reserved[6] = F32 (data + 50);
    return 0;
}

int
applanix_encode_msg22 (const applanix_msg22_t *msg, uint8_t * data, int length)
{
    int out_len = 60;

    if (out_len > length)
        return -1;

    memset (data, 0, out_len);
    memcpy (data, "$MSG", 4);
    ENCODE_U16 (data + 4, 22);
    ENCODE_U16 (data + 6, 52);
    ENCODE_U16 (data + 8, msg->transaction);
    ENCODE_F32 (data + 10, msg->DMI_scale);
    ENCODE_F32 (data + 14, msg->ref_to_DMI[0]);
    ENCODE_F32 (data + 18, msg->ref_to_DMI[1]);
    ENCODE_F32 (data + 22, msg->ref_to_DMI[2]);
    ENCODE_F32 (data + 26, msg->reserved[0]);
    ENCODE_F32 (data + 30, msg->reserved[1]);
    ENCODE_F32 (data + 34, msg->reserved[2]);
    ENCODE_F32 (data + 38, msg->reserved[3]);
    ENCODE_F32 (data + 42, msg->reserved[4]);
    ENCODE_F32 (data + 46, msg->reserved[5]);
    ENCODE_F32 (data + 50, msg->reserved[6]);

    memcpy (data + out_len - 2, "$#", 2);
    ENCODE_U16 (data + out_len - 4, applanix_compute_checksum (data));
    return out_len;
}

void
applanix_print_msg22 (FILE * f, const applanix_msg22_t * msg)
{
    fprintf (f, "  Transaction: %d\n", msg->transaction);
    fprintf (f, "  DMI scale: %.2f\n", msg->DMI_scale);
    fprintf (f, "  Ref. to DMI  X: %.3f  Y: %.3f  Z: %.3f\n",
            msg->ref_to_DMI[0], msg->ref_to_DMI[1], msg->ref_to_DMI[2]);
    fprintf (f, "\n");
}

int
applanix_decode_msg50 (applanix_msg50_t *msg, const uint8_t * data)
{
    msg->transaction = U16 (data + 8);
    msg->navigation_mode = data[10];
    return 0;
}

static const char * navigation_mode_desc[] = {
    "No operation",
    "Standby",
    "Navigate",
};

void
applanix_print_msg50 (FILE * f, const applanix_msg50_t * msg)
{
    fprintf (f, "  Transaction: %d\n", msg->transaction);
    fprintf (f, "  Navigation mode: %d (%s)\n", msg->navigation_mode,
            navigation_mode_desc[msg->navigation_mode]);
    fprintf (f, "\n");
}

int
applanix_encode_msg50 (const applanix_msg50_t *msg, uint8_t * data,
        int length)
{
    int out_len = 16;

    if (out_len > length)
        return -1;

    memset (data, 0, out_len);
    memcpy (data, "$MSG", 4);
    ENCODE_U16 (data + 4, 50);
    ENCODE_U16 (data + 6, 8);
    ENCODE_U16 (data + 8, msg->transaction);
    data[10] = msg->navigation_mode;
    data[11] = 0;

    memcpy (data + out_len - 2, "$#", 2);
    ENCODE_U16 (data + out_len - 4, applanix_compute_checksum (data));
    return out_len;
}

int
applanix_decode_msg52 (applanix_msg52_t *msg, const uint8_t * data)
{
    msg->transaction = U16 (data + 8);
    msg->num_groups = U16 (data + 10);
    if (msg->num_groups > 70)
        return -1;
    int i;
    for (i = 0; i < msg->num_groups; i++)
        msg->groups[i] = U16 (data + 12 + 2*i);
    msg->rate = U16 (data + 12 + 2*i);

    return 0;
}

int
applanix_encode_msg52 (const applanix_msg52_t *msg, uint16_t id,
        uint8_t * data, int length)
{
    int out_len = 8 + 10 + 2 * msg->num_groups;
    if (out_len % 4 != 0)
        out_len += 2;

    if (out_len > length)
        return -1;

    memset (data, 0, out_len);
    memcpy (data, "$MSG", 4);
    ENCODE_U16 (data + 4, id);
    ENCODE_U16 (data + 6, out_len - 8);
    ENCODE_U16 (data + 8, msg->transaction);
    ENCODE_U16 (data + 10, msg->num_groups);
    int i;
    for (i = 0; i < msg->num_groups; i++)
        ENCODE_U16 (data + 12 + 2*i, msg->groups[i]);
    ENCODE_U16 (data + 12 + 2 * msg->num_groups, msg->rate);

    memcpy (data + out_len - 2, "$#", 2);
    ENCODE_U16 (data + out_len - 4, applanix_compute_checksum (data));
    return out_len;
}

void
applanix_print_msg52 (FILE * f, const applanix_msg52_t * msg)
{
    fprintf (f, "  Transaction: %d\n", msg->transaction);
    fprintf (f, "  Groups: ");
    int i;
    for (i = 0; i < msg->num_groups; i++) {
        if (i > 0)
            fprintf (f, ", ");
        fprintf (f, "%d", msg->groups[i]);
    }
    fprintf (f, "\n  Data rate: %d Hz\n", msg->rate);
    fprintf (f, "\n");
}

int
applanix_encode_msg54 (const applanix_msg54_t *msg, uint8_t * data, int length)
{
    int out_len = 16;

    if (out_len > length)
        return -1;

    memset (data, 0, out_len);
    memcpy (data, "$MSG", 4);
    ENCODE_U16 (data + 4, 54);
    ENCODE_U16 (data + 6, 8);
    ENCODE_U16 (data + 8, msg->transaction);
    data[10] = msg->control;
    data[11] = 0;

    memcpy (data + out_len - 2, "$#", 2);
    ENCODE_U16 (data + out_len - 4, applanix_compute_checksum (data));
    return out_len;
}

int
applanix_encode_msg57 (const applanix_msg57_t *msg, uint8_t * data, int length)
{
    int out_len = 16;

    if (out_len > length)
        return -1;

    memset (data, 0, out_len);
    memcpy (data, "$MSG", 4);
    ENCODE_U16 (data + 4, 57);
    ENCODE_U16 (data + 6, 8);
    ENCODE_U16 (data + 8, msg->transaction);
    data[10] = msg->calib_action;
    data[11] = msg->calib_select;

    memcpy (data + out_len - 2, "$#", 2);
    ENCODE_U16 (data + out_len - 4, applanix_compute_checksum (data));
    return out_len;
}

static const char * grp_desc[] = {
    "Vehicle navigation solution",              // 1
    "Vehicle navigation performance metrics",   // 2
    "Primary GPS status",                       // 3
    "Time-tagged IMU data",                     // 4
    "Event 1 data",                             // 5
    "Event 2 data",                             // 6
    "PPS data",                                 // 7
    "Logging status",                           // 8    
    "GAMS solution status",                     // 9
    "General and FDIR status",                  // 10
    "Secondary GPS status",                     // 11
    "Auxiliary 1 GPS status",                   // 12
    "Auxiliary 2 GPS status",                   // 13
    "Calibrated installation parameters",       // 14
    "Time-tagged DMI data",                     // 15
    "User time status",                         // 17
    "IIN solution status",                      // 20
    "Base 1 GPS modem status",                  // 21
    "Base 2 GPS modem status",                  // 22
    "Auxiliary 1 GPS display data",             // 23
    "Auxiliary 2 GPS display data",             // 24
    "Integrated DGPS status",                   // 25
    "DGPS station database",                    // 26
    "Versions and statistics",                  // 99
    "Primary GPS data stream",                  // 10001
    "IMU data stream",                          // 10002
    "PPS data",                                 // 10003
    "Event 1 data",                             // 10004
    "Event 2 data",                             // 10005
    "DMI data stream",                          // 10006
    "Auxiliary 1 GPS data stream",              // 10007
    "Auxiliary 2 GPS data stream",              // 10008
    "Secondary GPS data stream",                // 10009
    "Base 1 GPS data stream",                   // 10011
    "Base 2 GPS data stream",                   // 10012
};

static const char * msg_desc[] = {
    "Acknowledge",                              // 0
    "General installation parameters",          // 20
    "GAMS installation parameters",             // 21
    "Aiding sensor installation parameters",    // 22
    "User accuracy specifications",             // 24
    "Zero-velocity update control",             // 25
    "Primary GPS set-up",                       // 30
    "Secondary GPS set-up",                     // 31
    "Set POS IP address",                       // 32
    "Event discretes set-up",                   // 33
    "COM port set-up",                          // 34
    "NMEA message select",                      // 35
    "Binary message select",                    // 36
    "Base GPS 1 set-up",                        // 37
    "Base GPS 2 set-up",                        // 38
    "Gravity correction",                       // 40
    "Integrated DGPS source control",           // 41
    "Navigation mode control",                  // 50
    "Display port control",                     // 51
    "Data port control",                        // 52
    "Logging port control",                     // 53
    "Save/restore parameters command",          // 54
    "Time synchronization control",             // 55
    "General data",                             // 56
    "Installation calibration control",         // 57
    "GAMS calibration control",                 // 58
    "Position fix control",                     // 60
    "Second data port control",                 // 61
    "Program control",                          // 90
    "GPS control",                              // 91
    "Integration diagnostics control",          // 92
    "Aiding sensor integration control",        // 93
};

const char *
applanix_grp_desc (int id)
{
    if (id <= 0)
        return NULL;
    if (id <= 15)
        return grp_desc[id-1];
    if (id <= 16)
        return NULL;
    if (id <= 17)
        return grp_desc[id-2];
    if (id <= 19)
        return NULL;
    if (id <= 26)
        return grp_desc[id-4];
    if (id <= 98)
        return NULL;
    if (id <= 99)
        return grp_desc[id-76];
    if (id <= 10000)
        return NULL;
    if (id <= 10012)
        return grp_desc[id-9977];
    return NULL;
}

const char *
applanix_msg_desc (int id)
{
    if (id <= 0)
        return msg_desc[id];
    if (id <= 19)
        return NULL;
    if (id <= 22)
        return msg_desc[id-19];
    if (id <= 23)
        return NULL;
    if (id <= 25)
        return msg_desc[id-20];
    if (id <= 29)
        return NULL;
    if (id <= 38)
        return msg_desc[id-24];
    if (id <= 39)
        return NULL;
    if (id <= 41)
        return msg_desc[id-25];
    if (id <= 49)
        return NULL;
    if (id <= 58)
        return msg_desc[id-33];
    if (id <= 59)
        return NULL;
    if (id <= 61)
        return msg_desc[id-34];
    if (id <= 89)
        return NULL;
    if (id <= 93)
        return msg_desc[id-62];
    return NULL;
}

#if 0
int 
applanix_grp1_to_imu_t(const applanix_grp1_t *grp1, 
					   imu_t *imu, uint64_t utime) 
{
	imu->utime = utime;
	imu->linear_accel[0] = grp1->acc_lon;
	imu->linear_accel[1] = -grp1->acc_trans;
	imu->linear_accel[2] = -grp1->acc_down;
	imu->rotation_rate[0] = to_radians(grp1->rate_lon);
	imu->rotation_rate[1] = -to_radians(grp1->rate_trans);
	imu->rotation_rate[2] = -to_radians(grp1->rate_down);
	imu->imu_serial_number = 1;

	quat_from_roll_pitch_yaw (to_radians(grp1->roll),-to_radians(grp1->pitch),
				  -to_radians(grp1->heading-grp1->wander),
				  imu->q);

	imu->has_heading = 1;
	imu->heading = to_radians(grp1->wander - 90);
	return 0;
}


int 
applanix_grp1_to_nmea_t(const applanix_grp1_t *grp1, 
						nmea_t *nm, uint64_t utime) 
{
    nm->utime=utime;
    // FAA Mode Indicator:
    char faa_mode ='N'; // N = Data Not Valid
	if (grp1->status<8) {
	  faa_mode='D'; // D = Differential Mode
	}
	//faa_mode='E'; // E = Estimated (dead-reckoning) mode
	//faa_mode='M'; // M = Manual Input Mode
	//faa_mode='S'; // S = Simulated Mode
	
	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	char loc_time[16];
	strftime(loc_time,16,"%H%M%S",tmp);
	char loc_date[16];
	strftime(loc_date,16,"%d%m%y",tmp);
	// FIX ME: HACK TO CORRECT FOR NAD83 => ITRF00 for sitevisit course for yoshi 5/23/07

	double lat_deg = floor (fabs (grp1->lat));
	double lon_deg = floor (fabs (grp1->lon));

	// $GPRMC,214345.00,V,4209.185245,N,07056.278929,W,0.00,0.0,280307,0.0,E,N*0F
	sprintf(nm->nmea,"$GPRMC,%s.00,%c,%02.0f%010.7f,%c,%03.0f%010.7f,%c,%03.3f,%09.5f,%s,0.0,N,%c*0F",
			loc_time, (grp1->status<8)?'A':'V',
			lat_deg, 60.0*(fabs(grp1->lat)-lat_deg),
			(grp1->lat>=0.0)?'N':'S',
			lon_deg, 60.0*(fabs(grp1->lon)-lon_deg),
			(grp1->lon>=0.0)?'E':'W',
			grp1->speed, grp1->heading, loc_date, faa_mode);
	
	// compute check sum.
	unsigned char cksum=0;
	unsigned char *ptr=(unsigned char *)nm->nmea+1;
	while (*ptr!='*') {
	    cksum^=*ptr;
	    ptr++;
	}
	sprintf((char *)ptr+1,"%02X",cksum);

	return 0;
}


// find the difference between two angles in degrees
// return the result in [-180.0,180]
double angle_diff(double a, double b) {
  double c = a-b;
  if (c>180.0)
    c-=360.0;
  else if (c<-180.0)
    c+=360.0;
  return c;
}


int 
applanix_grp1_to_speed(const applanix_grp1_t *grp1, double *speed) 
{
	if (abs(angle_diff(grp1->heading,grp1->track))<90.0)
        *speed = grp1->speed;
    else
        *speed = -grp1->speed;

	return 0;
}
#endif


int
applanix_nad83_itrf00_compute_constants(double R[3][3], double T[3], int64_t utime) 
{
    const double M_r=4.84813681e-9;
    const double T_t0[3]={+0.9956, -1.9013, -0.5215};
    const double dT[3]={+0.0007, -0.0007, +0.0005};
    const double E_t0[3]={+25.915, +9.426, +11.599};
    const double dE[3]={+0.067, -0.757, -0.051};
    const double S_t0=+0.62e-9;
    const double dS=-0.18e-9;
    const double t0=1997.00;

    // compute constants
    struct timeval tv;
    timestamp_to_timeval(utime, &tv);
    struct tm *tm = gmtime(&tv.tv_sec);
    double t = 1900.0+tm->tm_year+tm->tm_yday/366.0;

    T[0] = T_t0[0] + dT[0]*(t-t0);
    T[1] = T_t0[1] + dT[1]*(t-t0);
    T[2] = T_t0[2] + dT[2]*(t-t0);
    double Wx = (E_t0[0] + dE[0]*(t-t0))*M_r;
    double Wy = (E_t0[1] + dE[1]*(t-t0))*M_r;
    double Wz = (E_t0[2] + dE[2]*(t-t0))*M_r;
    double S = S_t0 + dS*(t-t0);

    R[0][0]=R[1][1]=R[2][2]=1.0+S;
    R[1][2]=Wx;
    R[2][1]=-Wx;
    R[0][2]=-Wy;
    R[2][0]=Wy;
    R[0][1]=-Wz;
    R[1][0]=Wz;

    printf("ti: %le\n",t);
    printf("T: %le %le %le\n",T[0],T[1],T[2]);
    printf("R0: %le %le %le\n",R[0][0],R[0][1],R[0][2]);
    printf("R1: %le %le %le\n",R[1][0],R[1][1],R[1][2]);
    printf("R2: %le %le %le\n",R[2][0],R[2][1],R[2][2]);


    return 0;
}

// all coords in meters.
int 
applanix_nad83_2_itrf00(double nad83[3], double itrf00[3], int64_t utime) 
{
    static int firsttime=1;
    static double R[3][3];
    static double T[3];

    // compute constants
    if (firsttime) {
        applanix_nad83_itrf00_compute_constants(R, T, utime); 
        firsttime=0;
    }

    itrf00[0]= -T[0] + R[0][0]*nad83[0] + R[1][0]*nad83[1] + R[2][0]*nad83[2];
    itrf00[1]= -T[1] + R[0][1]*nad83[0] + R[1][1]*nad83[1] + R[2][1]*nad83[2];
    itrf00[2]= -T[2] + R[0][2]*nad83[0] + R[1][2]*nad83[1] + R[2][2]*nad83[2];

    return 0;
}

// all coords in meters.
int 
applanix_itrf00_2_nad83(double itrf00[3], double nad83[3], int64_t utime) 
{
    static int firsttime=1;
    static double R[3][3];
    static double T[3];

    // compute constants
    if (firsttime) {
        applanix_nad83_itrf00_compute_constants(R, T, utime); 
        firsttime=0;
    }

    nad83[0]=T[0] + R[0][0]*itrf00[0] + R[0][1]*itrf00[1] + R[0][2]*itrf00[2];
    nad83[1]=T[1] + R[1][0]*itrf00[0] + R[1][1]*itrf00[1] + R[1][2]*itrf00[2];
    nad83[2]=T[2] + R[2][0]*itrf00[0] + R[2][1]*itrf00[1] + R[2][2]*itrf00[2];
    return 0;
}

uint16_t 
applanix_groupid (const uint8_t *data)
{
    return data[4] + (data[5]<<8);
}


uint16_t 
applanix_payload_length (const uint8_t *data)
{
    return data[6] + (data[7]<<8);
}

uint16_t
applanix_checksum (const uint8_t * data)
{
    int len = applanix_payload_length (data);
    return data[len+4] | (data[len+5] << 8);
}

uint16_t 
applanix_compute_checksum (const uint8_t *data)
{
    int payloadlen = applanix_payload_length(data);
    uint16_t sum = 0;

    for (int i = 0; i < 4+payloadlen; i+=2) {
        sum+=(data[i]+(data[i+1]<<8));
    }
    sum+=(data[6+payloadlen]+(data[7+payloadlen]<<8));

    return -sum&0x00FFFF;
}

struct applanix_msg90
{
    char start[4];             //Message start   4     char $MSG 
    uint16_t id;               //Message ID     2     ushort 90
    uint16_t len;              //Byte count     2     ushort 8
    uint16_t trans;            // Transaction number    2            
                               //Input:     Transaction number          N/A
                               //             Output: [65533, 65535]
     uint16_t control;      // Control        2     ushort
    // 000        Controller alive
    // 001        Terminate TCP/IP connection
    // 100        Reset GAMS
    // 101        Reset POS
    // 102        Shutdown POS
    // all other values are reserved

  uint16_t chksum;          //60 Checksum  2 ushort N/A N/A
  char end[2];              //62 Group end 2  char  $#  N/A

} __attribute__ ((packed));
typedef struct applanix_msg90 applanix_msg90_t;



int applanix_encode_msg90(uint8_t *data, int mode) 
{
    static uint16_t count=1;
    applanix_msg90_t msg;
    strncpy(msg.start,"$MSG",4);
    msg.id = 90;
    msg.len = 8;
    msg.trans = count++;
    msg.control = mode;
    strncpy(msg.end,"$#",2);
    msg.chksum = applanix_compute_checksum((uint8_t *)&msg);
    memcpy(data,&msg,sizeof(msg));
    return sizeof(msg);
}
