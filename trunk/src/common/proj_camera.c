#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "quaternion.h"

#include "proj_camera.h"
#include "small_linalg.h"
#include "quaternion.h"

#define dbg(args...) fprintf(stderr, args)
//#undef dbg
//#define dbg(args...)

proj_camera_t * 
proj_camera_create()
{
    dbg("proj_camera: create\n");
    proj_camera_t *s = (proj_camera_t*) calloc(1, sizeof(proj_camera_t) );

    s->rotation[0] = 1;
    s->rotation[1] = 0;
    s->rotation[2] = 0;
    s->rotation[3] = 0;
    s->translation[0] = 0;
    s->translation[1] = 0;
    s->translation[2] = 0;

    s->focal_length = 1;
    s->cop[0] = 0;
    s->cop[1] = 0;

    return s;
}

void 
proj_camera_destroy( proj_camera_t *s )
{
    memset( s, 0, sizeof(s) );
    free( s );
}

void
proj_camera_pixel_to_camera_ray( const proj_camera_t *s, double x, double y,
       double result[3] )
{
    // TODO model radial distortion

    // ray in camera frame
    result[0] = x - s->cop[0];
    result[1] = y - s->cop[1];
    result[2] = s->focal_length;

    // rotate and translate to world frame
    quat_rotate( s->rotation, result );
    result[0] += s->translation[0];
    result[1] += s->translation[1];
    result[2] += s->translation[2];

    // normalize
    vector_normalize_3d( result );
}

void
proj_camera_rotate_quat( proj_camera_t *s, const double quaternion[4] )
{
    double tmp[4];
    quat_mult( tmp, quaternion, s->rotation );
    s->rotation[0] = tmp[0];
    s->rotation[1] = tmp[1];
    s->rotation[2] = tmp[2];
    s->rotation[3] = tmp[3];
}

void 
proj_camera_rotate_euler( proj_camera_t *s, double roll, double pitch,
        double yaw )
{
    double q[4];
    quat_from_roll_pitch_yaw( roll, pitch, yaw, q );
    double tmp[4];
    quat_mult( tmp, s->rotation, q );
    s->rotation[0] = tmp[0];
    s->rotation[1] = tmp[1];
    s->rotation[2] = tmp[2];
    s->rotation[3] = tmp[3];
}
void
proj_camera_translate( proj_camera_t *s, double x, double y, double z )
{
    s->translation[0] += x;
    s->translation[1] += y;
    s->translation[2] += z;
}

