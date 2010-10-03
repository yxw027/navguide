#ifndef QUATERNION_H
#define QUATERNION_H

/**
 * SECTION:quaternion
 * @short_description: Quaternion multiplication and rotation.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Multiply quaternions a and b, storing the result in c.
 *
 * When composing rotations, the resulting quaternion represents 
 * first rotating by b, then rotation by a.
 */
void quat_mult (double * c, const double * a, const double * b);

void quat_rotate (const double * rot, double * v);
void quat_rotate_rev (const double * rot, double * v);

/** quat_from_angle_axis:
 *
 * populates a quaternion so that it represents a rotation of theta radians
 * about the axis <x,y,z>
 **/
void quat_from_angle_axis( double theta, const double axis[3], double q[4] );

void quat_to_angle_axis( const double q[4], double *theta, double axis[3] );

/**
 * converts a rotation from RPY representation into unit quaternion
 * representation
 *
 * rpy[0] = roll
 * rpy[1] = pitch
 * rpy[2] = yaw
 */
void quat_from_roll_pitch_yaw( double roll, double pitch, double yaw, 
        double *q );

/**
 * converts a rotation from unit quaternion representation to RPY
 * representation
 *
 * If any of roll, pitch, or yaw are NULL, then they are not set.
 *
 * rpy[0] = roll
 * rpy[1] = pitch
 * rpy[2] = yaw
 */
void quat_to_roll_pitch_yaw( const double *q, double *roll, 
        double *pitch,
        double *yaw);

/* These doesn't truly belong with the quaternion functions, but are useful
 * and sort of fits in here.
 */
void roll_pitch_yaw_to_angle_axis( const double rpy[3], double *angle,
        double axis[3] );

void angle_axis_to_roll_pitch_yaw( double angle, const double axis[3],
        double rpy[3] );

// runs some sanity checks
int quaternion_test();

#ifdef __cplusplus
}
#endif

#endif
