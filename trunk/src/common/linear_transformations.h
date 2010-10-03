#ifndef __linear_transformations_h__
#define __linear_transformations_h__

// convenience functions to populate matrices with simple linear
// transformations

#ifdef __cplusplus
extern "C" {
#endif

static inline void
lt_scale_matrix( double xscale, double yscale, double zscale, 
       double *m )
{
    m[0]  = xscale; m[1]  = 0; m[2]  = 0; m[3]  = 0;
    m[4]  = 0; m[5]  = yscale; m[6]  = 0; m[7]  = 0;
    m[8]  = 0; m[9]  = 0; m[10] = zscale; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

static inline void
lt_rotation_matrix_from_quaternion( const double *q, double *m )
{
    m[0] = q[0]*q[0] + q[1]*q[1] - q[2]*q[2] - q[3]*q[3];
    m[1] = 2 * (q[1]*q[2] - q[0]*q[3]);
    m[2] = 2 * (q[0]*q[2] + q[1]*q[3]);
    m[3] = 0;
    m[4] = 2 * (q[0]*q[3] + q[1]*q[2]);
    m[5] = q[0]*q[0] - q[1]*q[1] + q[2]*q[2] - q[3]*q[3];
    m[6] = 2 * (q[2]*q[3] - q[0]*q[1]);
    m[7] = 0;
    m[8] = 2 * (q[1]*q[3] - q[0]*q[2]);
    m[9] = 2 * (q[0]*q[1] + q[2]*q[3]);
    m[10] = q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3];
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

static inline void
lt_translation_matrix( double dx, double dy, double dz, double *m )
{
    m[0]  = 1; m[1]  = 0; m[2]  = 0; m[3]  = dx;
    m[4]  = 0; m[5]  = 1; m[6]  = 0; m[7]  = dy;
    m[8]  = 0; m[9]  = 0; m[10] = 1; m[11] = dz;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

#ifdef __cplusplus
}
#endif

#endif
