#ifndef __proj_camera_h__
#define __proj_camera_h__

#ifdef __cplusplus
extern "C" {
#endif

/* proj_camera represents a projective camera, and can be used for simple
 * camera geometry.
 * 
 * does not yet model radial distortion
 */
typedef struct _proj_camera {
    // intrinsics
    double focal_length;     // focal length
    double cop[2];           // center of projection [x, y]


    // extrinsics
    double rotation[4];      // unit quaternion that rotates from camera frame
                             // to world frame
    double translation[3];   // translation from camera frame to world frame


    // private.  do not touch or access.
    // TODO
} proj_camera_t;

// constructor
proj_camera_t * proj_camera_create();

// destructor
void proj_camera_destroy( proj_camera_t *s );

/* computes the ray, in world coordinate frame, that emanates from the focal
 * point through pixel <x,y>
 * 
 * resulting vector is of unit length.
 * 
 * if focal_length is 0, then result is undefined
 */
void proj_camera_pixel_to_camera_ray( const proj_camera_t *s, 
        double x, double y, double result[3] );

/* rotates the camera by with specified quaternion.  rotation is performed in
 * the world frame
 */
void proj_camera_rotate_quat( proj_camera_t *s, const double quaternion[4] );

/* TODO
 */
void proj_camera_rotate_euler( proj_camera_t *s, double roll, double pitch,
        double yaw );

/* translates the camera by the specified amounts.  translation is performed in
 * the world frame
 */
void proj_camera_translate( proj_camera_t *s, double x, double y, double z );

#ifdef __cplusplus
}
#endif

#endif
