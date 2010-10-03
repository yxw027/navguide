/*
 * This module computes dead-reckoning from IMU data.
 *
 * This code is borrowed from the MIT Darpa Urban Challenge code base.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#include <common/quaternion.h>
#include <glib.h>
#include <common/glib_util.h>
#include <common/config.h>

#include <lcm/lcm.h>


#include <lcmtypes/navlcm_imu_t.h>
#include <bot/bot_core.h>

#ifndef SQUARE
#define SQUARE(a) ( (a)*(a) )
#endif

typedef struct _State {

    botlcm_pose_t pose;

    uint8_t calibrate;
    int8_t fixup;
    uint8_t auto_calibrate;

    uint32_t consecutive_still;
    uint32_t calibrate_still_count;
    uint8_t is_still;

    uint8_t calibration_done;
    int calib_count;
    double gyro_calib[3];
    double gyro_calib_sum[3];
    double accel_calib[3];
    double accel_calib_sum[3];
    int64_t first_imu_time;
    double speed;

    int64_t last_imu_time;
    double last_q[4];
    double sum_yaw, native_yaw;
    double quaternion_rate[4];

    uint8_t calib_announce;
    uint8_t fixup_announce;

    lcm_t * lcm;
    Config * config;
    int printed_startup_config;
    GMainLoop *loop;
} State;

const double forward_vec[3] = { 1, 0, 0 };
const double body_to_imu_default[4] = { 1, 0, 0, 0 };
const double gps_body_position_default[3] = { 0, 0, 0 };

int verbose = 0;

static void
gyro_calibrate (const navlcm_imu_t * imu, State * s)
{
    int i;
    if (s->calib_count == 0) {
        s->first_imu_time = imu->utime;
        for (i = 0; i < 3; i++)
            s->gyro_calib_sum[i] = 0.0;
        for (i = 0; i < 3; i++)
            s->accel_calib_sum[i] = 0.0;
    }
    for (i = 0; i < 3; i++)
        s->gyro_calib_sum[i] += imu->rotation_rate[i];
    for (i = 0; i < 3; i++)
        s->accel_calib_sum[i] += imu->linear_accel[i];
    s->calib_count++;

    /* After 5 seconds, finish up */
    if (imu->utime - s->first_imu_time > 5000000) {
        for (i = 0; i < 3; i++)
            s->gyro_calib[i] = s->gyro_calib_sum[i] / s->calib_count;
        printf ("Calibrated biases: %f %f %f\n", s->gyro_calib[0],
                s->gyro_calib[1], s->gyro_calib[2]);
        for (i = 0; i < 3; i++)
            s->accel_calib[i] = s->accel_calib_sum[i] / s->calib_count;
        printf ("Acceleration: %f %f %f\n", s->accel_calib[0],
                s->accel_calib[1], s->accel_calib[2]);
        s->calibrate = 0;
        s->calibration_done = 1;
        s->calib_announce = 1;
        s->calibrate_still_count = 0;
        s->calib_count = 0;
        s->is_still = 0;
    }
}

static double
angle_clamp (double theta)
{
    if (theta > 4*M_PI || theta < -4*M_PI)
        fprintf (stderr, "Warning: theta (%f) is more than 2pi out of range\n",
                theta);
    if (theta > 2*M_PI)
        return theta - 2*M_PI;
    if (theta < -2*M_PI)
        return theta + 2*M_PI;
    return theta;
}

static void
imu_fixup_yaw (navlcm_imu_t * imu, State * s)
{
    if (s->last_imu_time == 0) {
        s->last_imu_time = imu->utime;
        memcpy (s->last_q, imu->q, 4 * sizeof (double));
        s->sum_yaw = 0.0;
        s->native_yaw = 0.0;
        return;
    }

    double q[4], fixup[4], rate[3];
    int64_t dt = imu->utime - s->last_imu_time;

    // Rotate the angular velocity vector into the global frame
    // and integrate the global frame's yaw rate (the global frame's
    // yaw rate is always in the plane perpendicular to gravity).
    memcpy (rate, imu->rotation_rate, 3 * sizeof (double));
    quat_rotate (imu->q, rate);
    if (s->consecutive_still < 30) {
        /* Only integrate yaw when we are moving */
        s->sum_yaw += rate[2] * dt / 1000000.0;
        s->sum_yaw = angle_clamp (s->sum_yaw);
    }

    // Compute q, the quaternion that rotates the previous orientation
    // into the current orientation.
    s->last_q[1] = -s->last_q[1];
    s->last_q[2] = -s->last_q[2];
    s->last_q[3] = -s->last_q[3];
    quat_mult (q, imu->q, s->last_q);

    // Compute the yaw component of q and integrate it.  Note that
    // this is not degenerate unlike most Euler angles because q is
    // guaranteed to be a very small rotation.
    s->native_yaw += atan2 (2*(q[1]*q[2] + q[0]*q[3]),
            (q[0]*q[0] + q[1]*q[1] - q[2]*q[2] - q[3]*q[3]));
    s->native_yaw = angle_clamp (s->native_yaw);

    // Compute a fixup quaternion that corrects for the difference
    // in yaw between sum_yaw and native_yaw in the global frame,
    // and apply the fixup to the orientation quaternion.
    fixup[0] = cos ((s->sum_yaw - s->native_yaw) / 2);
    fixup[1] = 0;
    fixup[2] = 0;
    fixup[3] = sin ((s->sum_yaw - s->native_yaw) / 2);
    quat_mult (q, fixup, imu->q);
    if (verbose) {
        printf ("fixup yaw: old %8.4f new %8.4f, quat: %8.4f %8.4f %8.4f %8.4f\n",
                s->native_yaw * 180.0 / M_PI,
                s->sum_yaw * 180.0 / M_PI,
                q[0], q[1], q[2], q[3]);
    }

    s->last_imu_time = imu->utime;
    memcpy (s->last_q, imu->q, 4 * sizeof (double));

    memcpy (imu->q, q, 4 * sizeof (double));

}

#define startup_printf(str) \
    do { \
        if (!s->printed_startup_config) \
            fprintf (stderr, str); \
    } while (0)

static int
emit_pose_state (State * s, navlcm_imu_t * imu)
{
    if (!s->calibration_done)
        return 0;
    
    // integrate rotation rate
    int64_t delta_usecs = imu->utime - s->last_imu_time;

    if (s->last_imu_time == 0) {
        s->last_imu_time = imu->utime;
        return 0;
    }

    s->last_imu_time = imu->utime;
    double fact = 1.0 * delta_usecs / 1000000;

    double q[4];
    quat_from_roll_pitch_yaw (imu->rotation_rate[0]*fact, imu->rotation_rate[1]*fact, imu->rotation_rate[2]*fact, q);

    //printf ("%.4f %.4f %.4f %.4f\n", q[0], q[1], q[2], q[3]);

    double qq[4];
    quat_mult (qq, s->pose.orientation, q);
    printf ("%.4f %.4f %.4f %.4f\n", qq[0], qq[1], qq[2], qq[3]);
    for (int i=0;i<4;i++)
        s->pose.orientation[i] = qq[i];

    s->pose.utime = imu->utime;

    botlcm_pose_t_publish ( s->lcm, "POSE", &s->pose );

    return 0;

    /* TODO: handle the case when the sensor frame and local frame
     * do not have the same orientation. */

    double body_to_imu[4];
    double gps_body_position[3];
    memcpy (body_to_imu, body_to_imu_default, 4 * sizeof (double));
    memcpy (gps_body_position, gps_body_position_default,
            3 * sizeof (double));

    s->printed_startup_config = 1;

    /* Transform the forward-pointing unit vector of the vehicle into
     * the local coordinate system */
    quat_mult (s->pose.orientation, imu->q, body_to_imu);
    double dir[3];
    memcpy (dir, forward_vec, 3 * sizeof (double));
    quat_rotate (s->pose.orientation, dir);

    /* Transform the rotation rates from the IMU coordinate frame to
     * the local coordinate system */
    memcpy (s->pose.rotation_rate, imu->rotation_rate, 3 * sizeof (double));
    quat_rotate (imu->q, s->pose.rotation_rate);

    /* HACK: Lock vehicle motion to Z=0 until we have a better measure of
     * the pitch. We do this by setting dir_z = 0 and
     * renormalizing the vector to unit length. */
    dir[2] = 0; 
    double norm = sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
    dir[0] /= norm;
    dir[1] /= norm;

    if (!s->pose.utime) {
        /* First time through */
        s->pose.pos[0] = 0;
        s->pose.pos[1] = 0;
        s->pose.pos[2] = 0;
    }
    else {
        double dt = (imu->utime - s->pose.utime) * 1e-6;
        double dist = s->speed * dt;

        /* Scale the forward-pointing vector by the distance traveled
         * and add it to our position estimate in micrometers. */
        int i;
        for (i = 0; i < 3; i++)
            s->pose.pos[i] += dir[i] * dist;
    }
    s->pose.utime = imu->utime;

    /* Scale the forward-pointing vector by the velocity as estimated
     * by the odometry system. */
    int i;
    for (i = 0; i < 3; i++)
        s->pose.vel[i] = dir[i] * s->speed;

    if (verbose) {
        printf ("output pos: %10.4f %10.4f %10.4f\n",
                (double)s->pose.pos[0],
                (double)s->pose.pos[1],
                (double)s->pose.pos[2]);
    }

    if (verbose)
        printf ("pose: %.4f %.4f %.4f\n", dir[0], dir[1], dir[2]);

    botlcm_pose_t_publish ( s->lcm, "POSE", &s->pose );

    return 0;
}

static void
on_imu (const lcm_recv_buf_t *buf, const char *channel, 
                           const navlcm_imu_t *imu,
                           void *user)
{
    navlcm_imu_t imu_out;
    int i;
    int force_fixup = 0;

    State *s = (State*)user;

    /* Calibrate at startup if the user requested it */
    if (s->calibrate) {
        gyro_calibrate (imu, s);
        return;
    }

    if (s->is_still) {
        s->consecutive_still++;
        s->calibrate_still_count++;

        /* Wait 3 seconds after stopping and then start a calibration */
        if (s->auto_calibrate && s->calibrate_still_count > 300)
            gyro_calibrate (imu, s);
    }
    else {
        s->calibrate_still_count = 0;
        s->consecutive_still = 0;
        s->calib_count = 0;
    }

    memcpy (&imu_out, imu, sizeof (navlcm_imu_t));
    if (s->calibration_done) {
        if (!s->calib_announce) {
            printf ("Using calibrated gyro biases\n");
            s->calib_announce = 1;
        }
        for (i = 0; i < 3; i++)
            imu_out.rotation_rate[i] -= s->gyro_calib[i];
    }
    if (s->fixup == 1 || force_fixup) {
        if (!s->fixup_announce) {
            printf ("Performing yaw fixup\n");
            s->fixup_announce = 1;
        }
        imu_fixup_yaw (&imu_out, s);
    }

    emit_pose_state (s, &imu_out);

    return;
}

int
main (int argc, char ** argv)
{
    int c;
    State s;

    // so that redirected stdout won't be insanely buffered.
    setlinebuf(stdout);

    memset (&s, 0, sizeof (State));

    s.fixup = -1;
    while ((c = getopt (argc, argv, "hvcf:oa")) >= 0) {
        switch (c) {
            case 'v':
                verbose = 1;
                break;
            case 'c':
                s.calibrate = 1;
                break;
            case 'a':
                s.auto_calibrate = 1;
                break;
            case 'f':
                s.fixup = atoi (optarg);
                break;
            case 'h':
            case '?':
                fprintf (stderr, "Usage: %s [-v] [-c] [-f]\n\
Options:\n\
 -v     Be verbose\n\
 -c     Calibrate the IMU's gyro biases for 5 seconds at startup\n\
 -a     Auto-calibrate IMU biases any time the vehicle is stationary\n\
 -f N   Enable the fixup of the IMU's quaternion in the yaw direction by\n\
        integrating the gyro's ourselves.  N=1 On, N=0 Off (default)\n\
 -h     This help message\n", argv[0]);
                return 1;
        }
    }

    s.lcm = lcm_create (NULL);
    s.speed = .0;
    s.loop = g_main_loop_new(NULL, FALSE);
    s.is_still = 1;

    s.last_imu_time = .0;

    s.pose.orientation[0] = 1.0;
    for (int i=1;i<4;i++)
        s.pose.orientation[i] = .0;

    s.config = config_parse_default ();
    if (!s.config)
        return 1;
    
    // attach lcm to main loop
    glib_mainloop_attach_lcm (s.lcm);

    navlcm_imu_t_subscribe (s.lcm, "IMU", on_imu, &s);

    signal_pipe_glib_quit_on_kill (s.loop);

    // run main loop
    g_main_loop_run (s.loop);
    
    // cleanup
    g_main_loop_unref (s.loop);

    return 0;
}
