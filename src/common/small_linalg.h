#ifndef __small_linalg_h__
#define __small_linalg_h__

// convenience functions for small linear algebra operations

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline double 
matrix_determinant_3x3d( const double *m )
{
    return m[0]*(m[4]*m[8] - m[5]*m[7]) 
         - m[1]*(m[3]*m[8] - m[5]*m[6]) 
         + m[2]*(m[3]*m[7] - m[4]*m[6]);
}

// returns 0 on success, -1 if matrix is singular
static inline int 
matrix_inverse_3x3d( const double *m, double *inverse )
{
    double det = matrix_determinant_3x3d( m );
    if( det == 0 ) return -1;
    double det_inv = 1.0 / det;
    inverse[0] = det_inv * (m[4]*m[8] - m[5]*m[7]);
    inverse[1] = det_inv * (m[2]*m[7] - m[1]*m[8]);
    inverse[2] = det_inv * (m[1]*m[5] - m[2]*m[4]);
    inverse[3] = det_inv * (m[5]*m[6] - m[3]*m[8]);
    inverse[4] = det_inv * (m[0]*m[8] - m[2]*m[6]);
    inverse[5] = det_inv * (m[2]*m[3] - m[0]*m[5]);
    inverse[6] = det_inv * (m[3]*m[7] - m[4]*m[6]);
    inverse[7] = det_inv * (m[1]*m[6] - m[0]*m[7]);
    inverse[8] = det_inv * (m[0]*m[4] - m[1]*m[3]);
    return 0;
}

static inline void 
matrix_vector_multiply_3x3_3d( const double *m, const double *v, 
        double *result )
{
    result[0] = m[0]*v[0] + m[1]*v[1] + m[2]*v[2];
    result[1] = m[3]*v[0] + m[4]*v[1] + m[5]*v[2];
    result[2] = m[6]*v[0] + m[7]*v[1] + m[8]*v[2];
}

static inline void
matrix_vector_multiply_4x4_4d( const double *m, const double *v,
        double *result )
{
    result[0] = m[0]*v[0] + m[1]*v[1] + m[2]*v[2] + m[3]*v[3];
    result[1] = m[4]*v[0] + m[5]*v[1] + m[6]*v[2] + m[7]*v[3];
    result[2] = m[8]*v[0] + m[9]*v[1] + m[10]*v[2] + m[11]*v[3];
    result[3] = m[12]*v[0] + m[13]*v[1] + m[14]*v[2] + m[15]*v[3];
}

static inline void
vector_add_3d( const double *v1, const double *v2, double *result ) 
{
    result[0] = v1[0] + v2[0];
    result[1] = v1[1] + v2[1];
    result[2] = v1[2] + v2[2];
}

/**
 * vector_subtract_3d:
 *
 * computes v1 - v2
 */
static inline void
vector_subtract_3d( const double *v1, const double *v2, double *result ) 
{
    result[0] = v1[0] - v2[0];
    result[1] = v1[1] - v2[1];
    result[2] = v1[2] - v2[2];
}

static inline double
vector_dot_2d( const double *v1, const double *v2 )
{
    return v1[0]*v2[0] + v1[1]*v2[1];
}

static inline double 
vector_dot_3d( const double *v1, const double *v2 )
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

static inline void
vector_cross_3d( const double *v1, const double *v2, double *result )
{
    result[0] = v1[1]*v2[2] - v1[2]*v2[1];
    result[1] = v1[2]*v2[0] - v1[0]*v2[2];
    result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

static inline double
vector_magnitude_2d( const double *v )
{
    return sqrt(v[0]*v[0] + v[1]*v[1]);
}

static inline double
vector_magnitude_3d( const double *v )
{
    return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static inline void
vector_normalize_2d( double *v )
{
    double mag = vector_magnitude_2d( v );
    v[0] /= mag;
    v[1] /= mag;
}

static inline void
vector_normalize_3d( double *v )
{
    double mag = vector_magnitude_3d( v );
    v[0] /= mag;
    v[1] /= mag;
    v[2] /= mag;
}

static inline void
vector_scale_2d( double *v, double s )
{
    v[0] *= s;
    v[1] *= s;
}

static inline void
vector_scale_3d( double *v, double s )
{
    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
}

static inline double
vector_angle_2d( const double *v1, const double *v2 )
{
    double mag1 = vector_magnitude_2d( v1 );
    double mag2 = vector_magnitude_2d( v2 );
    double dot = vector_dot_2d( v1, v2 );

    double costheta = dot / ( mag1 * mag2 );
    if( costheta > 1 ) return 0;
    double theta = acos( costheta );

    if ( ( v1[0]*v2[1] - v1[1]*v2[0] ) < 0 ) {
        return -theta;
    }

    return theta;
}

static inline void
matrix_transpose_4x4d( const double *m, double *result )
{
    result[0] = m[0];
    result[1] = m[4];
    result[2] = m[8];
    result[3] = m[12];
    result[4] = m[1];
    result[5] = m[5];
    result[6] = m[9];
    result[7] = m[13];
    result[8] = m[2];
    result[9] = m[6];
    result[10] = m[10];
    result[11] = m[14];
    result[12] = m[3];
    result[13] = m[7];
    result[14] = m[11];
    result[15] = m[15];
}

#ifdef __cplusplus
}
#endif

#endif
