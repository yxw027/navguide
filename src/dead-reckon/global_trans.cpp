#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>

#include <common/nmea.h>
#include <common/quaternion.h>

#include <lc/lc.h>

#include <lcmtypes/pose_state_t.h>
#include <lcmtypes/nmea_t.h>
#include <lcmtypes/global_trans_t.h>

#include "global_trans.h"

#define REQ 6378.135
#define RPO 6356.750

/* speed in m/s that the vehicle must be traveling at in order for the
 * Kalman filter to receive heading updates. */
#define MIN_SPEED 1.0

/* GPS-measured heading noise stddev in rad */
//#define GPS_HEADING_STD (5*M_PI/180)

/* GPS-measured position noise stddev in meters */
#define GPS_POSITION_STD 0.1
/* IMU heading noise density in rad/s/rtHz */
#define IMU_HEADING_NOISE_DENSITY   0.04

/* Assume 0 ms latency between GPS measurement and when we receive it. */
#define GPS_LATENCY 0

static void set_global_origin_gps( GlobalTrans *s, 
        const gps_state_t * gps );
static void set_global_origin_local( GlobalTrans *s,
        const pose_state_t * pose );

static int on_gps( const gps_nmea_t * gd, const gps_state_t * gps,
        GlobalTrans * );

#define MAX_LOCAL_POSES 100

struct _GlobalTrans {
    lc_t *lc;
    gps_nmea_t *nmea;

    global_trans_t trans;

    double gps_body_position[3];
    pose_state_t local_poses[MAX_LOCAL_POSES];
    int lpos_start, lpos_num;
    int64_t last_utime;

    int has_direct_heading;
    double direct_heading;
    double direct_heading_var;
};

GlobalTrans *
global_trans_create (lc_t * lc)
{
    GlobalTrans * s = (GlobalTrans *) calloc (1, sizeof(GlobalTrans));

    s->lc = lc;
    assert( lc );

    s->nmea = nmea_new();

    nmea_subscribe( s->nmea, lc, "NMEA", (gps_state_handler_t) on_gps, s );

    return s;
}

void 
global_trans_destroy( GlobalTrans *s )
{
    nmea_unsubscribe( s->nmea, s->lc, "NMEA", (gps_state_handler_t) on_gps, s );
    nmea_free( s->nmea );

    memset( s, 0, sizeof(GlobalTrans) );
    free( s );
}

// ================= LC handlers ================
int 
global_trans_new_pose (GlobalTrans * s, const pose_state_t * ps,
        const double * gps_body_position, int has_direct_heading,
        double direct_heading, double direct_heading_var)
{
    int i;

    if (s->lpos_num < MAX_LOCAL_POSES) {
        i = (s->lpos_start + s->lpos_num) % MAX_LOCAL_POSES;
        s->lpos_num++;
    }
    else {
        i = s->lpos_start;
        s->lpos_start = (s->lpos_start + 1) % MAX_LOCAL_POSES;
    }
    memcpy (s->local_poses + i, ps, sizeof(pose_state_t) );
    memcpy (s->gps_body_position, gps_body_position, 3 * sizeof (double));

    s->has_direct_heading = has_direct_heading;
    s->direct_heading = direct_heading;
    s->direct_heading_var = direct_heading_var;
     
    return 0;
}

static double
angle_diff (double a1, double a2)
{
    double d = a1 - a2;

    if (d < -M_PI) {
        while (d < -M_PI)
            d += 2*M_PI;
    }
    else if (d > M_PI) {
        while (d > M_PI)
            d -= 2*M_PI;
    }
    return d;
}

#define sq(x) ((x)*(x))

/* gps is (lat, lon, elev) */
static void
linearize_gps(GlobalTrans * s, const double * gps, double * xyz )
{
    double dlon = (gps[1] - s->trans.gps_origin_lon) * M_PI / 180;
    double dlat = (gps[0] - s->trans.gps_origin_lat) * M_PI / 180;

    xyz[0] = sin(dlon) * s->trans.radius *
        cos(s->trans.gps_origin_lat * M_PI / 180);
    xyz[1] = sin(dlat) * s->trans.radius;
    xyz[2] = gps[2] - s->trans.gps_origin_elev;
}

static int
update_heading (GlobalTrans * s,
        const gps_state_t * gps, const pose_state_t * pose, int64_t gps_utime)
{
    int64_t dt = gps_utime - s->last_utime;

    /* For this to work, the previous GPS fix should be no older than
     * 1.2 seconds. */
    if (s->last_utime == 0 || (dt > 1200000LL) || (dt <= 0LL))
        return -1;

    double pvec[3] = {
        pose->pos[0] - s->trans.local_gps_origin[0],
        pose->pos[1] - s->trans.local_gps_origin[1],
        0,
    };

    double speed_local_sq = (pvec[0]*pvec[0] + pvec[1]*pvec[1]) *
        sq(1000000.0 / dt);
    if (speed_local_sq < MIN_SPEED*MIN_SPEED)
        return -1;

    double gnew[3] = {gps->lat, gps->lon, gps->elev};
    double gvec[3];
    linearize_gps (s, gnew, gvec);
    gvec[2] = 0;

    double gps_dist_sq = gvec[0]*gvec[0] + gvec[1]*gvec[1];
    double speed_gps_sq = gps_dist_sq * sq(1000000.0 / dt);
    if (speed_gps_sq < MIN_SPEED*MIN_SPEED)
        return -1;

    double heading_diff = atan2 (pvec[1], pvec[0]) - atan2 (gvec[1], gvec[0]);

#if 0
    printf ("update %f state %f std %f dt %"PRId64"\n",
            angle_diff (heading_diff, s->trans.heading_diff) * 180/M_PI,
            s->trans.heading_diff * 180/M_PI,
            sqrt(s->trans.heading_diff_var)*180/M_PI,
            dt);
#endif

    if (s->trans.utime == 0) {
        /* If this is our first time through, initialize the Kalman filter. */
        s->trans.heading_diff_var = sq(GPS_POSITION_STD) / gps_dist_sq;
        s->trans.heading_diff = heading_diff;
    }
    else {
        // predicted covariance
        s->trans.heading_diff_var += sq(IMU_HEADING_NOISE_DENSITY) *
            (gps_utime - s->trans.utime) / 1000000.0;
        // Kalman gain
        double K = s->trans.heading_diff_var /
            (s->trans.heading_diff_var + sq(GPS_POSITION_STD)/gps_dist_sq);

        //printf ("meas %f\n",
        //        angle_diff (heading_diff, s->trans.heading_diff) * 180 / M_PI);
        // state update
        s->trans.heading_diff = s->trans.heading_diff +
            K * angle_diff (heading_diff, s->trans.heading_diff);
        // covariance update
        s->trans.heading_diff_var = (1 - K) * s->trans.heading_diff_var;
    }

    s->trans.utime = gps_utime;
    return 0;
}

int
pose_to_gps_origin (GlobalTrans * s,
        const pose_state_t * pose, pose_state_t * out)
{
    double offset[3];
    memcpy (offset, s->gps_body_position, 3 * sizeof (double));
    memcpy (out, pose, sizeof (pose_state_t));

    /* Convert from body frame to local frame */
    quat_rotate (pose->orientation, offset);

    out->pos[0] += offset[0];
    out->pos[1] += offset[1];
    out->pos[2] += offset[2];
    return 0;
}

int
on_gps( const gps_nmea_t * gd, const gps_state_t * gps, 
       GlobalTrans *s ) 
{
    int updated_filter = 0;
    int is_still = 0;
    int64_t gps_utime = gps->recv_utime - GPS_LATENCY;

    if (!s->lpos_num)
        goto done;

    /* Find the previous pose state that is closest in time to this
     * GPS measurement.  We know the GPS has more latency than the
     * IMU because it is shoving a lot more data across the USB bus. */
    pose_state_t * pose = NULL;
    int i;
    for (i = 0; i < s->lpos_num; i++) {
        pose = s->local_poses + (i + s->lpos_start) % MAX_LOCAL_POSES;

        if (i == 0 && pose->utime > gps_utime) {
            fprintf (stderr, "Warning: GPS time was older than oldest "
                    "pose state by %"PRId64" us\n", pose->utime - gps_utime);
            goto done;
        }
        
        if (pose->utime >= gps_utime)
            break;
    }

    if (i == s->lpos_num && (gps_utime - pose->utime) > 15000) {
        fprintf (stderr, "Warning: GPS time was newer than pose "
                "state by %"PRId64" us\n", gps_utime - pose->utime);
        goto done;
    }

    pose_state_t newpose;
    /* Convert the local frame reference point to the location of
     * the gps reading. */
    pose_to_gps_origin (s, pose, &newpose);

    if (newpose.vel[0] == 0 && newpose.vel[1] == 0 && newpose.vel[2] == 0)
        is_still = 1;

    if (gps->status != GPS_STATUS_LOCK && 
        gps->status != GPS_STATUS_DGPS_LOCK)
        goto done;

    if (update_heading (s, gps, &newpose, gps_utime) == 0)
        updated_filter = 1;

    set_global_origin_gps( s, gps );
    set_global_origin_local( s, &newpose );
    s->last_utime = gps_utime;

    if (s->has_direct_heading) {
        global_trans_t gt;
        memcpy (&gt, &s->trans, sizeof (global_trans_t));
        gt.heading_diff = s->direct_heading;
        gt.heading_diff_var = s->direct_heading_var;
        gt.utime = gps_utime;
        global_trans_t_lc_publish (s->lc, "GLOBAL_TRANS", &gt);
    }

done:
    if (s->trans.utime != 0) {
        /* If the filter didn't get updated by a measurement, just
         * perform a Kalman update step by itself so that the covariance
         * grows accordingly.  Only do this if the car is in motion. */
        if (!updated_filter) {
            if (!is_still)
                s->trans.heading_diff_var += sq(IMU_HEADING_NOISE_DENSITY) *
                    (gps_utime - s->trans.utime) / 1000000.0;
            s->trans.utime = gps_utime;
        }
        if (!s->has_direct_heading)
            global_trans_t_lc_publish (s->lc, "GLOBAL_TRANS", &s->trans);
    }

    return 0;
}

// ================ utility functions ================

void 
set_global_origin_gps( GlobalTrans *s, 
        const gps_state_t * gps )
{
    // latitude should always be between -90 (90 deg south) and +90 (90 deg
    // north)
    assert( gps->lat > -90 && gps->lat < 90 );
    assert( gps->lon > -360 && gps->lon < 360 );

    s->trans.gps_origin_lat = gps->lat;
    s->trans.gps_origin_lon = gps->lon;
    s->trans.gps_origin_elev = gps->elev;

    double latrad = M_PI / 180 * gps->lat;

    s->trans.radius = REQ*RPO /
        sqrt(REQ*REQ - (REQ*REQ - RPO*RPO) * pow(cos(latrad), 2));
    s->trans.radius *= 1000;
}

void
set_global_origin_local( GlobalTrans *s, 
        const pose_state_t *pose )
{
    s->trans.local_gps_origin[0] = pose->pos[0];
    s->trans.local_gps_origin[1] = pose->pos[1];
    s->trans.local_gps_origin[2] = pose->pos[2];
}

