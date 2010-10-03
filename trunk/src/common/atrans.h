#ifndef __ar_atrans_h__
#define __ar_atrans_h__

#include <bot/bot_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ATrans:
 *
 * ATrans is intended to replace CTrans.  It should be a one-stop shop for
 * vehicle pose information and coordinate transformations.
 */
typedef struct _ATrans ATrans;

ATrans * atrans_new(lcm_t *lcm, BotConf *config);

void atrans_destroy(ATrans * atrans);

/**
 * compute the latest rigid body transformation from one coordinate frame to
 * another.
 *
 * Returns: 1 on success, 0 on failure
 */
int atrans_get_trans(ATrans *atrans, const char *from_frame,
        const char *to_frame, BotTrans *result);

/**
 * compute the rigid body transformation from one coordinate frame
 * to another.
 *
 * Returns: 1 on success, 0 on failure
 */
int atrans_get_trans_with_utime(ATrans *atrans, const char *from_frame,
        const char *to_frame, int64_t utime, BotTrans *result);

/**
 * Returns: 1 if the requested transformation is availabe, 0 if not
 */
int atrans_have_trans(ATrans *atrans, const char *from_frame,
        const char *to_frame);

// convenience function
int atrans_get_trans_mat_3x4(ATrans *atrans, const char *from_frame,
        const char *to_frame, double mat[12]);

// convenience function
int atrans_get_trans_mat_3x4_with_utime(ATrans *atrans, 
        const char *from_frame, const char *to_frame, int64_t utime, 
        double mat[12]);

// convenience function
int atrans_get_trans_mat_4x4(ATrans *atrans, const char *from_frame,
        const char *to_frame, double mat[16]);

// convenience function
int atrans_get_trans_mat_4x4_with_utime(ATrans *atrans, 
        const char *from_frame, const char *to_frame, int64_t utime, 
        double mat[16]);


/**
 * Transforms a vector from one coordinate frame to another
 *
 * Returns: 1 on success, 0 on failure
 */
int atrans_transform_vec(ATrans *atrans, const char *from_frame,
        const char *to_frame, const double src[3], double dst[3]);

/**
 * Rotates a vector from one coordinate frame to another.  Does not apply
 * any translations.
 *
 * Returns: 1 on success, 0 on failure
 */
int atrans_rotate_vec(ATrans *atrans, const char *from_frame,
        const char *to_frame, const double src[3], double dst[3]);

/**
 * Retrieves the number of transformations available for the specified link.
 * Only valid for <from_frame, to_frame> pairs that are directly linked.  e.g.
 * <body, local> is valid, but <camera, local> is not.
 */
int atrans_get_n_trans(ATrans *atrans, const char *from_frame,
        const char *to_frame, int nth_from_latest);

/**
 * @nth_from_latest:  0 corresponds to most recent transformation update
 * @btrans: may be NULL
 * @timestamp: may be NULL
 *
 * Returns: 1 on success, 0 on failure
 */
int atrans_get_nth_trans(ATrans *atrans, const char *from_frame,
        const char *to_frame, int nth_from_latest, 
        BotTrans *btrans, int64_t *timestamp);

// convenience function to get the vehicle position in the local frame.
int atrans_vehicle_pos_local(ATrans *atrans, double pos[3]);

/**
 * Returns: 1 on success, 0 on failure
 */
int atrans_get_local_pose(ATrans *atrans, botlcm_pose_t *pose);

/**
 * Returns: 1 on success, 0 on failure
 */
int atrans_get_local_pos_heading(ATrans *atrans, double pos[3], double *heading);

#ifdef __cplusplus
}
#endif

#endif
